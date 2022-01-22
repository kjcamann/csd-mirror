"""GDB pretty-printers for CSD.
"""

import enum
import gdb
import gdb.xmethod
import re

_csd_printer_name = 'csd_pretty_printer'
_csd_xmethod_name = 'csd_xmethods'

def _remove_generics(typename):
  if type(typename) is gdb.Type:
    typename = typename.name or typename.tag or str(typename)
  match = re.match('^([^<]+)', typename)
  return match.group(1)

_nttpIntegralSuffix = {
  'long' : 'l',
  'long long' : 'll',
  'unsigned int' : 'u',
  'unsigned long' : 'ul',
  'unsigned long long' : 'ull'
}

def _get_entry_extractor_typename(ty):
  """Return the adjusted typename of an entry extractor for the purpose of
  performing symbol lookups.

  offset_extractor takes, as its third argument, a non-type template parameter
  of type `std::size_t`. When gcc prints the NTTP argument, it prints it as just
  a number, e.g., "8". In the symbol name, however, it must appear as something
  like "8ul" (unsigned long) because the symbol must encode the type according
  to the ABI rules.
  """
  templateName = _remove_generics(ty.strip_typedefs().name)
  if templateName != 'csg::offset_extractor':
    return ty.strip_typedefs().name
  offset = ty.template_argument(2)
  assert type(offset) is gdb.Value, 'offset_extractor template arg 2 not an NTTP?'
  suffix = _nttpIntegralSuffix.get(offset.type.name, None)
  fixedOffset = f'{offset}{suffix}' if suffix else str(offset)
  return f'csg::offset_extractor<{ty.template_argument(0)}, ' \
         f'{ty.template_argument(1)}, {fixedOffset}>'

def _lookup_entry_ref_codec_functions(elementTy, entryTy, entryExTy,
                                      entryRefUnionTy):
  """To iterate over CSD lists in the debugger, we need access to the functions
  entry_ref_codec<...>::get_entry and entry_ref_codec<...>::get_value, which
  are looked up using this helper.
  """
  entryExTyName = _get_entry_extractor_typename(entryExTy)
  entryRefCodecClassName = \
      f'csg::detail::entry_ref_codec<{entryTy}, {elementTy}, {entryExTyName}>'

  def lookupEntryRefCodecSymbol(fnName):
    """Look up symbol for entry_ref_codec<...> static member functions."""
    symName = f'{entryRefCodecClassName}::{fnName}'
    sym, _ = gdb.lookup_symbol(symName)
    if not sym or not sym.is_function:
      raise Exception(f'required symbol {symName} does not exist or is not a function')
    return sym

  getEntryFnName = f'get_entry({entryExTyName} &, {entryRefUnionTy})'
  getEntrySym = lookupEntryRefCodecSymbol(getEntryFnName)

  getValueFnName = f'get_value({entryRefUnionTy})'
  getValueSym = lookupEntryRefCodecSymbol(getValueFnName)

  return getEntrySym.value(), getValueSym.value()

class EntryRefPrinter:
  """Printer for csg::entry_ref_union<EntryType, T>"""
  def __init__(self, value):
    self.value = value
    self.addrVal = self.value['offset']['m_address']

    if int(self.addrVal) & 0x1:
      # A tagged address; this means we're pointing at a T value.
      ptrType = self.value.type.template_argument(1).pointer()
      self.childValue = (self.addrVal - 1).cast(ptrType)
      self.label = str(ptrType)
    else:
      # An untagged address; this means we're pointing directly an entry_type<T>
      ptrType = self.value.type.template_argument(0).pointer()
      self.childValue = self.addrVal.cast(ptrType)
      self.label = str(ptrType)

  def children(self):
    yield self.label, self.childValue

class ListPrinter:
  """Printer for all CSD list types."""
  class ListIterator:
    def __init__(self, nextEntryRef, entryEx, getEntryFn, getValueFn,
                 stopEntryRefValue):
      self.nextEntryRef = nextEntryRef
      self.entryEx = entryEx
      self.getEntryFn = getEntryFn
      self.getValueFn = getValueFn
      self.count = 0
      self.stopEntryRefValue = stopEntryRefValue

    def __iter__(self):
      return self

    def __next__(self):
      self.count += 1
      if self.isFinished():
        raise StopIteration
      item = self.getValueFn(self.nextEntryRef).referenced_value()
      curEntry = self.getEntryFn(self.entryEx, self.nextEntryRef).dereference()
      self.nextEntryRef = curEntry['next']
      return f'[{self.count}]', item

    def isFinished(self):
      return int(self.nextEntryRef['offset']['m_address']) == self.stopEntryRefValue

  def __init__(self, value):
    self.value = value
    self.entryEx = self.value['m_entryExtractor']

    qualListTyName = _remove_generics(value.type.strip_typedefs())
    assert qualListTyName.startswith('csg::')
    self.listTyName = qualListTyName[5:]
    self.isProxy = self.listTyName.endswith('_proxy')
    self.isTailQ = self.listTyName.startswith('tailq_')

    self.fwdHead = self.value['m_head']
    if self.isProxy:
      self.fwdHead = self.fwdHead.referenced_value()
    listHeadEntry = self.fwdHead['m_endEntry'] if self.isTailQ else \
        self.fwdHead['m_headEntry']
    stopEntryRefValue = int(listHeadEntry.address) if self.isTailQ else 0
    firstEntryRef = listHeadEntry['next']

    baseCls = self.value.type.fields()[0].type
    entryRefUnionTy = firstEntryRef.type

    self.elementTy = baseCls.template_argument(0)
    self.entryTy = entryRefUnionTy.template_argument(0)
    self.entryExTy = self.entryEx.type.strip_typedefs()

    getEntryFn, getValueFn = \
        _lookup_entry_ref_codec_functions(self.elementTy, self.entryTy,
                                          self.entryExTy, entryRefUnionTy)

    self.iterator = self.ListIterator(firstEntryRef, self.entryEx, getEntryFn,
                                      getValueFn, stopEntryRefValue)

  def to_string(self):
    desc = f'csg::{self.listTyName} of {self.elementTy}'
    size = self.fwdHead['m_sz']
    if size.type.code == gdb.TYPE_CODE_INT:
      desc += f', size = {size}'
    elif self.iterator.isFinished():
      desc += " (empty)"
    if self.entryEx.type.fields():
      entryExTyName = self.entryEx.type.strip_typedefs()
      desc += f', extractor {entryExTyName} = {self.entryEx}'
    return desc

  def children(self):
    return self.iterator

  def display_hint(self):
    return 'array'

class CSDPrettyPrinter:
  def __init__(self, name):
    self.name = name
    self.enabled = True

    self.lookup = {
      'slist_head' : ListPrinter,
      'slist_proxy' : ListPrinter,
      'stailq_head' : ListPrinter,
      'stailq_proxy' : ListPrinter,
      'tailq_head' : ListPrinter,
      'tailq_proxy' : ListPrinter,
      'entry_ref_union' : EntryRefPrinter,
    }

  def __call__(self, value):
    """Return the pretty printer for a gdb.Value"""

    canonType = value.type.strip_typedefs()
    if canonType.code not in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
      return None

    typename = canonType.name or canonType.tag or str(canonType)
    if not typename.startswith('csg::'):
      return None

    baseName = _remove_generics(typename[5:])
    printer = self.lookup.get(baseName, None)
    return printer(value) if printer else None

def _register_csd_printers(event):
  progspace = event.new_objfile.progspace

  if not getattr(progspace, _csd_printer_name, False):
    print("Loading csd pretty-printers")
    gdb.printing.register_pretty_printer(progspace,
        CSDPrettyPrinter(_csd_printer_name))
    setattr(progspace, _csd_printer_name, True)

def _unregister_csd_printers(event):
  progspace = event.progspace

  if getattr(progspace, _csd_printer_name, False):
    for printer in progspace.pretty_printers:
      if getattr(printer, "name", "none") == _csd_printer_name:
        progspace.pretty_printers.remove(printer)
        setattr(progspace, _csd_printer_name, False)
        break

def register_csd_pretty_printers():
  """Register event handlers to load csd pretty-printers."""
  gdb.events.new_objfile.connect(_register_csd_printers)
  gdb.events.clear_objfiles.connect(_unregister_csd_printers)
