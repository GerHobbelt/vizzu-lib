export type CPointer = number
export type CString = CPointer
export type CException = CPointer
export type CTypeInfo = CPointer
export type CSnapshotPtr = CPointer
export type CAnimationPtr = CPointer
export type CChartPtr = CPointer
export type CCanvasPtr = CPointer
export type CEventPtr = CPointer
export type CFunction = CPointer
export type CRecordPtr = CPointer
export type CRecordValue = CPointer
export type CPointPtr = CPointer
export type CArrayPtr = CPointer
export type CColorGradientPtr = CPointer

export type PtrType =
  | 'i1'
  | 'i8'
  | 'i16'
  | 'i32'
  | 'i64'
  | 'float'
  | 'double'
  | 'i1*'
  | 'i8*'
  | 'i16*'
  | 'i32*'
  | 'i64*'
  | 'float*'
  | 'double*'

export interface CExceptionInfo {
  get_type(): CTypeInfo
}
export interface CExceptionInfoConstructor {
  new (ptr: CPointer): CExceptionInfo
}

export interface ModuleOptions {
  locateFile?: (path: string) => string
}

export interface Renderer {
  setCursor(name: CString): void
  frameBegin(): void
  frameEnd(): void
  frameBegin(): void
  frameEnd(): void
  setClipRect(x: number, y: number, sizex: number, sizey: number): void
  setClipCircle(x: number, y: number, radius: number): void
  setClipPolygon(): void
  setBrushColor(r: number, g: number, b: number, a: number): void
  setLineColor(r: number, g: number, b: number, a: number): void
  setLineWidth(width: number): void
  setFont(font: CString): void
  setDropShadowBlur(radius: number): void
  setDropShadowColor(r: number, g: number, b: number, a: number): void
  setDropShadowOffset(x: number, y: number): void
  endDropShadow(): void
  endPolygon(): void
  addPoint(x: number, y: number): void
  addBezier(c0x: number, c0y: number, c1x: number, c1y: number, x: number, y: number): void
  endPolygon(): void
  rectangle(x: number, y: number, sizex: number, sizey: number): void
  circle(x: number, y: number, radius: number): void
  line(x1: number, y1: number, x2: number, y2: number): void
  textBoundary(text: CString, sizeX: CPointer, sizeY: CPointer): void
  text(x: number, y: number, sizex: number, sizey: number, text: CString): void
  setBrushGradient(
    x1: number,
    y1: number,
    x2: number,
    y2: number,
    stopCount: number,
    stops: CColorGradientPtr
  ): void
  transform(a: number, b: number, c: number, d: number, e: number, f: number): void
  save(): void
  restore(): void
}

export interface CVizzu {
  // decorations
  callback: (task: CFunction, obj: CPointer) => void
  renders: { [key: CPointer]: Renderer }

  // members
  HEAPU8: Uint8Array

  // exported runtime functions
  _malloc(len: number): CPointer
  _free(cpath: CPointer): void
  getValue(ptr: CPointer, type: PtrType): number
  setValue(ptr: CPointer, value: number, type: PtrType): void
  stringToUTF8(str: string, buffer: CPointer, len: number): void
  UTF8ToString(ptr: CString): string
  addFunction<T>(func: T, sig: string): CFunction
  removeFunction(cfunc: CFunction): void
  ExceptionInfo: CExceptionInfoConstructor

  // exported functions
  _callback(task: CFunction, obj: CPointer): void
  _vizzu_createChart(): CChartPtr
  _vizzu_createCanvas(): CCanvasPtr
  _vizzu_pointerDown(
    chart: CChartPtr,
    canvas: CCanvasPtr,
    pointerId: number,
    x: number,
    y: number
  ): void
  _vizzu_pointerUp(
    chart: CChartPtr,
    canvas: CCanvasPtr,
    pointerId: number,
    x: number,
    y: number
  ): void
  _vizzu_pointerMove(
    chart: CChartPtr,
    canvas: CCanvasPtr,
    pointerId: number,
    x: number,
    y: number
  ): void
  _vizzu_pointerLeave(chart: CChartPtr, canvas: CCanvasPtr, pointerId: number): void
  _vizzu_wheel(chart: CChartPtr, canvas: CCanvasPtr, delta: number): void
  _vizzu_setLogging(enable: boolean): void
  _vizzu_update(
    chart: CChartPtr,
    canvas: CCanvasPtr,
    width: number,
    height: number,
    renderControl: number
  ): void
  _vizzu_errorMessage(exceptionPtr: CException, typeinfo: CTypeInfo): CString
  _vizzu_version(): CString
  _data_addDimension(chart: CChartPtr, name: CString, categories: CArrayPtr, count: number): void
  _data_addMeasure(
    chart: CChartPtr,
    name: CString,
    unit: CString,
    values: CArrayPtr,
    count: number
  ): void
  _data_addRecord(chart: CChartPtr, cells: CArrayPtr, count: number): void
  _data_metaInfo(chart: CChartPtr): CString
  _record_getValue(record: CRecordPtr, column: CString): CRecordValue
  _chart_store(chart: CChartPtr): CSnapshotPtr
  _chart_restore(chart: CChartPtr, snapshot: CSnapshotPtr): void
  _chart_anim_store(chart: CChartPtr): CAnimationPtr
  _chart_anim_restore(chart: CChartPtr, anim: CAnimationPtr): void
  _object_free(handle: CPointer): void
  _style_getList(): CString
  _style_setValue(chart: CChartPtr, path: CString, value: CString): void
  _style_getValue(chart: CChartPtr, path: CString, computed: boolean): CString
  _chart_getList(): CString
  _chart_getValue(chart: CChartPtr, path: CString): CString
  _chart_setValue(chart: CChartPtr, path: CString, value: CString): void
  _chart_setFilter(chart: CChartPtr, filter: CFunction, deleter: CFunction): void
  _chart_animate(chart: CChartPtr, callback: CFunction): void
  _chart_relToCanvasCoords(chart: CChartPtr, rx: number, ry: number): CPointPtr
  _chart_canvasToRelCoords(chart: CChartPtr, x: number, y: number): CPointPtr
  _chart_setKeyframe(chart: CChartPtr): void
  _chart_markerData(chart: CChartPtr, id: number): CString
  _addEventListener(chart: CChartPtr, name: CString, callback: CFunction): void
  _removeEventListener(chart: CChartPtr, name: CString, callback: CFunction): void
  _event_preventDefault(event: CEventPtr): void
  _anim_control(chart: CChartPtr, command: CString, param: CString): void
  _anim_setValue(chart: CChartPtr, path: CString, value: CString): void
}

export declare const Module: CVizzu
