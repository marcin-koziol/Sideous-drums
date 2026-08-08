dpf_add_plugin(sideous-drums-multi
  TARGETS lv2 vst3 clap jack
  UI_TYPE cairo
  FILES_DSP
      SideousDrumsMultiPlugin.cpp
  FILES_UI
      ../common/SideousDrumsUI.cpp)

target_include_directories(sideous-drums-multi PUBLIC ".")
