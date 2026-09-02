source_file = [
    'tvgBgfxCommon.h',
    'tvgBgfxCompositor.h',
    'tvgBgfxContext.h',
    'tvgBgfxGeometry.h',
    'tvgBgfxRenderData.h',
    'tvgBgfxRenderer.h',
    'tvgBgfxRenderTarget.h',
    'tvgBgfxShaderTypes.h',
    'tvgBgfxTessellator.h',
    'tvgBgfxTextureMgr.h',
    'tvgBgfxCompositor.cpp',
    'tvgBgfxContext.cpp',
    'tvgBgfxRenderData.cpp',
    'tvgBgfxRenderer.cpp',
    'tvgBgfxRenderTarget.cpp',
    'tvgBgfxTessellator.cpp',
    'tvgBgfxTextureMgr.cpp'
]

# bgfx/bimg/bx 静态库由顶层构建系统统一链接（CMake: BGFX_WASM_LIBRARIES）。
# meson 侧仅收集源文件与头文件包含路径；如需独立 meson 构建，
# 可在此处通过 subproject('bgfx') 或 dependency('bgfx') 补充依赖。
bgfx_dep = []

engine_dep += [declare_dependency(
    dependencies        : bgfx_dep,
    include_directories : include_directories('.'),
    sources             : source_file
)]
