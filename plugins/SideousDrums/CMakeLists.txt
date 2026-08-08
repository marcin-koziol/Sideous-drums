dpf_add_plugin(sideous-drums
  TARGETS lv2 vst3 clap jack
  UI_TYPE cairo
  FILES_DSP
      SideousDrumsPlugin.cpp
  FILES_UI
      ../common/SideousDrumsUI.cpp)

target_include_directories(sideous-drums PUBLIC ".")
