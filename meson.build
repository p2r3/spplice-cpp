project('spplice', 'cpp')

if get_option('USE_QT6') 
  qt = import('qt6')
  qt_dep = dependency('qt6', modules: ['Core', 'Widgets', 'Gui', 'Concurrent'])
else
  qt = import('qt5')
  qt_dep = dependency('qt5', modules: ['Widgets', 'Concurrent'])
endif

duktape = dependency('duktape')
curl = dependency('libcurl')
libarchive = dependency('libarchive')

ui = qt.compile_ui(sources : files([
  'ui/MainWindow.ui',
  'ui/PackageItem.ui',
  'ui/ErrorDialog.ui',
  'ui/PackageInfo.ui',
  'ui/Repositories.ui',
  'ui/Settings.ui',
]))

moc_files = qt.compile_moc(headers : files([
  'ui/mainwindow_extend.h',
  'tools/package.h',
]))

resources = qt.compile_resources(
  sources: [
    'resources.qrc'
  ]
)

executable(
  'SppliceCPP',
  sources : [
    moc_files,
    ui,
    'ui/mainwindow_extend.cpp',
    'main.cpp',
    'globals.cpp',
    'tools/curl.cpp',
    'tools/update.cpp',
    'tools/qt.cpp',
    'tools/install.cpp',
    'tools/package.cpp',
    'tools/repo.cpp',
    'tools/js.cpp',
    'tools/netcon.cpp',
    'tools/merge.cpp',
    resources,
  ],
  dependencies: [
    qt_dep,
    duktape,
    curl,
    libarchive,
  ],
  install: true
)
