cmake_minimum_required(VERSION 3.30)
project(thorvg VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

# 添加 UTF-8 支持
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 配置参数
# Renders
option(THORVG_ENABLE_CPU_ENGINE "Enable cpu rasterizer engine supoort" ON)
option(THORVG_ENABLE_GL_ENGINE "Enable gl rasterizer engine support" ON)

# Multi-Tasking
option(THORVG_ENABLE_THREADS "Enable thread support for parallel rendering" ON)
option(THORVG_ENABLE_PARTIAL_RENDER "Enable partial rendering (render sub-regions of the frame" ON)
option(THORVG_ENABLE_OPENMP "Enable OpenMP support for parallel post-effects" ON)

# Loaders
option(THORVG_ENABLE_JPG_LOADER "Enable JPEG image loading support" ON)
option(THORVG_USE_EXTERNAL_JPG "Use external libturbojpeg for JPG loading (fallback to built-in)" OFF)

option(THORVG_ENABLE_PNG_LOADER "Enable PNG image loading support" ON)
option(THORVG_USE_EXTERNAL_PNG "Use external libpng for PNG loading (fallback to built-in)" OFF)

option(THORVG_ENABLE_WEBP_LOADER "Enable WebP image loading support" ON)
option(THORVG_USE_EXTERNAL_WEBP "Use external libwebp for WebP loading (fallback to built-in)" OFF)

option(THORVG_ENABLE_LOTTIE_LOADER "Enable Lottie image loading support" ON)
option(THORVG_ENABLE_SVG_LOADER "Enable SVG image loading support" ON)
option(THORVG_ENABLE_MEDIA_LOADER "Enable Media loading support" ON)

option(THORVG_ENABLE_TTF_LOADER "Enable TTF loading support" ON)
option(THORVG_ENABLE_OTF_LOADER "Enable OTF loading support" ON)

# Savers
option(THORVG_ENABLE_GIF_SAVER "Enable Gif save support" ON)

# Vectorization
option(THORVG_ENABLE_SIMD "Enable CPU Vectorization(SIMD) support" ON)

option(THORVG_ENABLE_LOGGING "Enable logging-related compiler diagnostics" OFF)

set(THORVG_SANITIZE "none" CACHE STRING
    "Sanitizer mode for Thorvg (none, address, undefined, thread)")
set_property(CACHE THORVG_SANITIZE PROPERTY STRINGS
    none address undefined thread)

# File IO
option(THORVG_ENABLE_FILE_IO "Enable file io support" ON)

set(THORVG_LIBRARY_TYPE "SHARED" CACHE STRING
    "Library type override for Thorvg (SHARED, STATIC, OBJECT, or empty to follow BULD_SHARED_LIBS)")

set(THORVG_SIMD_TYPE "none")

if(${THORVG_ENABLE_SIMD})
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i386|i686)")
        set(THORVG_SIMD_TYPE "avx")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        set(THORVG_SIMD_TYPE "neon-aarch")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7|armv8)")
        set(THORVG_SIMD_TYPE "neon-arm")
    endif()
endif()

if(THORVG_LIBRARY_TYPE)
    if(THORVG_LIBRARY_TYPE STREQUAL "SHARED")
        set(THORVG_BUILD_SHARED_LIBRARY TRUE)
    else()
        set(THORVG_BUILD_SHARED_LIBRARY FALSE)
    endif()
else()
    set(THORVG_BUILD_SHARED_LIBRARY ${BUILD_SHARED_LIBS})
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/loaders/raw
    ${CMAKE_SOURCE_DIR}/src/loaders
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(PROJECT_HEADERS
    ${CMAKE_SOURCE_DIR}/inc/thorvg.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgAllocator.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgArray.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgColor.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgCommon.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgCompressor.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgInlist.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgLock.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgMap.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgMath.h
    ${CMAKE_SOURCE_DIR}/src/common/tvgStr.h
    ${CMAKE_SOURCE_DIR}/src/loaders/raw/tvgRawLoader.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgAccessor.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgAnimation.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgCanvas.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgFill.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgLoader.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgLoaderMgr.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgPaint.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgPicture.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgRender.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgSaveModule.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgScene.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgShape.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgTaskScheduler.h
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgText.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h # 占位
)

set(PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/tvgCompressor.cpp
    ${CMAKE_SOURCE_DIR}/src/common/tvgMath.cpp
    ${CMAKE_SOURCE_DIR}/src/common/tvgStr.cpp
    ${CMAKE_SOURCE_DIR}/src/loaders/raw/tvgRawLoader.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgAccessor.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgAnimation.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgCanvas.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgFill.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgInitializer.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgLoaderMgr.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgPaint.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgPicture.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgRender.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgSaver.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgScene.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgShape.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgTaskScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/renderer/tvgText.cpp
)

if(${THORVG_ENABLE_CPU_ENGINE})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwCommon.h
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRasterAvx.h
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRasterC.h
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRasterNeon.h
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRasterTexmap.h
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRenderer.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwBlendOp.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwFill.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwImage.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwMemPool.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwPostEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRaster.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwRle.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwShape.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwStroke.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/cpu_engine/tvgSwUtil.cpp
    )
endif()

if(${THORVG_ENABLE_GL_ENGINE})
    find_package(OpenGL REQUIRED)

    include_directories(
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGl.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlCommon.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlEffect.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlGpuBuffer.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlProgram.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderer.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderPass.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderTarget.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderTask.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlShader.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlShaderSrc.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlSolidBatch.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlStateCache.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlStencilCoverBatch.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlTessellator.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlTextureMgr.h
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/tvgGpuCommon.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGl.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlGeometry.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlGpuBuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlProgram.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderPass.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderTarget.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlRenderTask.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlShader.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlShaderSrc.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlSolidBatch.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlStateCache.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlStencilCoverBatch.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlTessellator.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/gl/tvgGlTextureMgr.cpp
        ${CMAKE_SOURCE_DIR}/src/renderer/gpu_engine/tvgGpuCommon.cpp
    )
endif()

if(${THORVG_USE_EXTERNAL_JPG})
    find_package(libjpeg-turbo REQUIRED)

    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/external_jpg
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/external_jpg/tvgJpgLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/external_jpg/tvgJpgLoader.cpp
    )
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/jpg
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/jpg/tvgJpgd.h
        ${CMAKE_SOURCE_DIR}/src/loaders/jpg/tvgJpgLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/jpg/tvgJpgd.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/jpg/tvgJpgLoader.cpp
    )
endif()

if(${THORVG_USE_EXTERNAL_PNG})
    find_package(PNG REQUIRED)

    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/external_png
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/external_png/tvgPngLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/external_png/tvgPngLoader.cpp
    )
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/png
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/png/tvgLodePng.h
        ${CMAKE_SOURCE_DIR}/src/loaders/png/tvgPngLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/png/tvgLodePng.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/png/tvgLodePngUtil.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/png/tvgPngLoader.cpp
    )
endif()

if(${THORVG_USE_EXTERNAL_WEBP})
    find_package(WebP REQUIRED)

    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/external_webp
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/external_webp/tvgWebpLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/external_webp/tvgWebpLoader.cpp
    )
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/webp
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/alphai.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/common.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/decode_vp8.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/vp8i.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/vp8li.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/webpi.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/dsp.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/lossless.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/yuv.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/bit_reader_inl.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/bit_reader.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/color_cache.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/endian_inl.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/huffman.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/quant_levels_dec.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/random.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/rescaler.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/utils.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/webp/decode.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/webp/format_constants.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/webp/types.h
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/tvgWebpLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/alpha.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/buffer.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/frame.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/io.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/quant.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/tree.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/vp8.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/vp8l.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dec/webp.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/alpha_processing.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/argb.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/cpu.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/dec_clip_tables.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/dec.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/filters.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/lossless.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/rescaler.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/upsampling.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/dsp/yuv.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/bit_reader.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/color_cache.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/huffman.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/quant_levels_dec.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/random.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/utils/rescaler.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/webp/tvgWebpLoader.cpp
    )
endif()

if(${THORVG_ENABLE_LOTTIE_LOADER})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jcontext
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-alloc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-error-messages.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-errors.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-extended-info.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-gc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-globals.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-number.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-init-finalize.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-lcache.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-literal-storage.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-module.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-property-hashmap.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-bigint64array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-bigint64array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-biguint64array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-biguint64array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-float32array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-float32array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int16array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int16array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int32array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int32array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int8array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int8array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-helpers.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-prototype-template.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-template.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint16array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint16array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint32array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint32array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-aggregateerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-aggregateerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-prototype-unscopables.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-arraybuffer-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-arraybuffer.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-from-sync-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-function-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-function.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator-function.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-atomics.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-bigint-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-bigint.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-boolean-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-boolean.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-dataview-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-dataview.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-date-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-date.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-error-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-error.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-evalerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-evalerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-function-prototype.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-function-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-function.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator-function.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-global.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-handlers.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-handlers.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers-macro-defines.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers-macro-undefs.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-internal-routines-template.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-intrinsic.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-math.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-number-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-number.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-object-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-object.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-promise-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-promise.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-proxy.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-rangeerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-rangeerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-referenceerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-referenceerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-reflect.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp-string-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-shared-arraybuffer-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-shared-arraybuffer.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string-iterator-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-symbol-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-symbol.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-syntaxerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-syntaxerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-type-error-thrower.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-typeerror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-typeerror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-urierror-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-urierror.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakmap-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakmap.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakref-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakref.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakset-prototype.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakset.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtins-internal.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtins.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtins.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-arguments-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-array-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-arraybuffer-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-async-generator-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-atomics-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-big-uint.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-bigint-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-bigint.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-boolean-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-comparison.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-container-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-conversion.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-dataview-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-eval.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-exceptions.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-function-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-iterator-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-jobqueue.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-lex-env.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-number-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-objects-general.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-objects.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-promise-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-proxy-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-reference.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-regexp-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-shared-arraybuffer-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-string-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-symbol-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-typedarray-object.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerry-config.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerryscript-compiler.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerryscript-core.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerryscript-port.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerryscript-types.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/include/jerryscript.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jcontext/jcontext.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem/jmem-allocator-internal.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem/jmem.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt-bit-fields.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt-libc-includes.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt-types.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-char-helpers.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-globals.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-magic-strings.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-magic-strings.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-strings.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-unicode-conversions-sup.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-unicode-conversions.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-unicode-folding.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-unicode-ranges-sup.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-unicode-ranges.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/byte-code.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/common.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-lexer.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-limits.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-scanner.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-internal.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-tagged-template-literal.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-scanner-internal.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/parser-errors.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/parser-error-messages.inc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-bytecode.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-compiler-context.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-compiler.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-parser.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-token.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/opcodes.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm-defines.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm-stack.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/error/en.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/error/error.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/biginteger.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/clzll.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/diyfp.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/dtoa.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/ieee754.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/itoa.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/meta.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/pow10.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/regex.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/stack.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/strfunc.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/strtod.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/internal/swap.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/msinttypes/inttypes.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/msinttypes/stdint.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/allocators.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/cursorstreamwrapper.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/document.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/encodedstream.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/encodings.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/filereadstream.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/filewritestream.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/fwd.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/istreamwrapper.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/memorybuffer.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/memorystream.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/ostreamwrapper.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/pointer.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/prettywriter.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/rapidjson.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/reader.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/schema.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/stream.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/stringbuffer.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/uri.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/rapidjson/writer.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/thorvg_lottie.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieBuilder.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieCommon.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieExpressions.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieInterpolator.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieLoader.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieModel.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieModifier.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieParser.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieParserHandler.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieProperty.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieRenderPooler.h
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieTween.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/api/jerryscript.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-alloc.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-errors.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-extended-info.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-gc.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-collection.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-conversion.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-errol.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-external-pointers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-number.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-string.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers-value.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-helpers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-init-finalize.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-lcache.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-literal-storage.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-module.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/base/ecma-property-hashmap.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-bigint64array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-bigint64array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-biguint64array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-biguint64array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-float32array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-float32array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int16array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int16array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int32array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int32array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int8array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-int8array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-helpers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-typedarray.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint16array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint16array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint32array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint32array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/typedarray/ecma-builtin-uint8clampedarray.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-aggregateerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-aggregateerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-prototype-unscopables.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-array.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-arraybuffer-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-arraybuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-from-sync-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-function-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-function.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator-function.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-generator.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-async-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-atomics.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-bigint-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-bigint.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-boolean-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-boolean.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-dataview-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-dataview.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-date-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-date.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-error-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-error.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-evalerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-evalerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-function-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-function.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator-function.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-generator.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-global.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-handlers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers-date.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers-error.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers-sort.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-helpers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-intrinsic.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-map.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-math.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-number-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-number.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-object-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-promise-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-promise.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-proxy.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-rangeerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-rangeerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-referenceerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-referenceerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-reflect.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp-string-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-regexp.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-set.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-shared-arraybuffer-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-shared-arraybuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string-iterator-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-string.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-symbol-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-symbol.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-syntaxerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-syntaxerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-type-error-thrower.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-typeerror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-typeerror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-urierror-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-urierror.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakmap-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakmap.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakref-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakref.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakset-prototype.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtin-weakset.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/builtin-objects/ecma-builtins.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-arguments-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-array-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-arraybuffer-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-async-generator-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-atomics-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-big-uint.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-bigint-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-bigint.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-boolean-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-comparison.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-container-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-conversion.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-dataview-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-eval.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-exceptions.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-function-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-get-put-value.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-iterator-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-jobqueue.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-lex-env.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-number-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-objects-general.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-objects.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-promise-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-proxy-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-reference.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-regexp-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-shared-arraybuffer-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-string-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-symbol-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/ecma/operations/ecma-typedarray-object.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jcontext/jcontext.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem/jmem-allocator.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem/jmem-heap.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jmem/jmem-poolman.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt-fatals.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/jrt/jrt-logging.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-char-helpers.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-magic-strings.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/lit/lit-strings.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/byte-code.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/common.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-lexer.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-expr.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-mem.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-module.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-statm.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-tagged-template-literal.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-parser-util.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-scanner.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-scanner-ops.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/js-scanner-util.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/js/parser-errors.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-bytecode.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-compiler.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/parser/regexp/re-parser.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/opcodes-ecma-arithmetics.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/opcodes-ecma-bitwise.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/opcodes-ecma-relational-equality.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/opcodes.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm-stack.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm-utils.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-core/vm/vm.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/jerryscript/jerry-port/common/jerry-port-context.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieAnimation.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieExpressions.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieInterpolator.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieModel.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieModifier.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieParser.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieParserHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/tvgLottieProperty.cpp
    )
endif()

if(${THORVG_ENABLE_SVG_LOADER})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/svg
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgBuilder.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgCommon.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgCssStyle.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgLoader.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgPath.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgUtil.h
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgXmlParser.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgCssStyle.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgPath.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgSvgUtil.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/svg/tvgXmlParser.cpp
    )
endif()

if(${THORVG_ENABLE_MEDIA_LOADER})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/media
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/media/thorvg_media.h
        ${CMAKE_SOURCE_DIR}/src/loaders/media/tvgMediaLoader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/media/tvgVideo.cpp
    )

    if(APPLE)
        list(APPEND PROJECT_HEADERS
            ${CMAKE_SOURCE_DIR}/src/loaders/media/apple/tvgAvfMediaLoader.h
        )

        list(APPEND PROJECT_SOURCES
            ${CMAKE_SOURCE_DIR}/src/loaders/media/apple/tvgAvfMediaLoader.mm
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        list(APPEND PROJECT_HEADERS
            ${CMAKE_SOURCE_DIR}/src/loaders/media/web/tvgWebMediaLoader.h
        )

        list(APPEND PROJECT_SOURCES
            ${CMAKE_SOURCE_DIR}/src/loaders/media/web/tvgWebMediaLoader.cpp
        )
    else()
        list(APPEND PROJECT_SOURCES
            ${CMAKE_SOURCE_DIR}/src/loaders/media/tvgMediaLoader.cpp
        )
    endif()
endif()

if(${THORVG_ENABLE_TTF_LOADER} OR ${THORVG_ENABLE_OTF_LOADER})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/loaders/sfnt
    )

    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgSfntLoader.h
        ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgSfntReader.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgSfntLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgSfntReader.cpp
    )

    if(${THORVG_ENABLE_TTF_LOADER})
        list(APPEND PROJECT_HEADERS
            ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgTtfReader.h
        )

        list(APPEND PROJECT_SOURCES
            ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgTtfReader.cpp
        )
    endif()

    if(${THORVG_ENABLE_OTF_LOADER})
        list(APPEND PROJECT_HEADERS
            ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgOtfReader.h
        )

        list(APPEND PROJECT_SOURCES
            ${CMAKE_SOURCE_DIR}/src/loaders/sfnt/tvgOtfReader.cpp
        )
    endif()
endif()

if(${THORVG_ENABLE_GIF_SAVER})
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/savers/gif
    )
    
    list(APPEND PROJECT_HEADERS
        ${CMAKE_SOURCE_DIR}/src/savers/gif/tvgGifEncoder.h
        ${CMAKE_SOURCE_DIR}/src/savers/gif/tvgGifSaver.h
    )

    list(APPEND PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/savers/gif/tvgGifEncoder.cpp
        ${CMAKE_SOURCE_DIR}/src/savers/gif/tvgGifSaver.cpp
    )
endif()

add_library(${PROJECT_NAME} ${THORVG_LIBRARY_TYPE}
    ${PROJECT_HEADERS}
    ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/inc>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_VERSION_STRING="${PROJECT_VERSION}")

set(THORVG_SANITIZE_VALUES none address undefined thread)
if(NOT THORVG_SANITIZE IN_LIST THORVG_SANITIZE_VALUES)
    message(FATAL_ERROR
        "Invalid THORVG_SANITIZE='${THORVG_SANITIZE}'. "
        "Expected one of: none, address, undefined, thread.")
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

set(THORVG_CLANG_CL FALSE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"
        OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(THORVG_CLANG_CL TRUE)
endif()

if(THORVG_CLANG_CL)
    if(THORVG_SIMD_TYPE STREQUAL "avx")
        target_compile_options(${PROJECT_NAME} PRIVATE "/clang:-mavx")
    elseif(THORVG_SIMD_TYPE STREQUAL "neon-arm")
        target_compile_options(${PROJECT_NAME} PRIVATE "/clang:-mfpu=neon")
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE
        "/clang:-Wno-unknown-pragmas"
    )

    if(NOT THORVG_ENABLE_LOGGING)
        target_compile_options(${PROJECT_NAME} PRIVATE "/clang:-Wdouble-promotion")
    endif()

    if(THORVG_SANITIZE STREQUAL "none")
        target_compile_options(${PROJECT_NAME} PRIVATE
            "/clang:-fno-exceptions"
            "/clang:-fno-rtti"
            "/clang:-fno-math-errno"
            "/clang:-Woverloaded-virtual"
            "/clang:-fno-stack-protector"
            "/clang:-fno-unwind-tables"
            "/clang:-fno-asynchronous-unwind-tables"
        )
    endif()
elseif(NOT MSVC)
    if(THORVG_SIMD_TYPE STREQUAL "avx")
        target_compile_options(${PROJECT_NAME} PRIVATE "-mavx")
    elseif(THORVG_SIMD_TYPE STREQUAL "neon-arm")
        target_compile_options(${PROJECT_NAME} PRIVATE "-mfpu=neon")
    endif()

    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wno-unknown-pragmas"
    )

    if(NOT THORVG_ENABLE_LOGGING)
        target_compile_options(${PROJECT_NAME} PRIVATE "-Wdouble-promotion")
    endif()

    if(THORVG_SANITIZE STREQUAL "none")
        target_compile_options(${PROJECT_NAME} PRIVATE
            "-fno-exceptions"
            "-fno-rtti"
            "-fno-stack-protector"
            "-fno-math-errno"
            "-fno-unwind-tables"
            "-fno-asynchronous-unwind-tables"
            "-Woverloaded-virtual"
        )
    endif()
endif()

if(${THORVG_ENABLE_SIMD})
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i386|i686)")
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_AVX_VECTOR_SUPPORT)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7|armv8)")
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_NEON_VECTOR_SUPPORT)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_NEON_VECTOR_SUPPORT)
    endif()
endif()

if(${THORVG_BUILD_SHARED_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PUBLIC TVG_EXPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TVG_BUILD)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC TVG_STATIC)
endif()

if(${THORVG_ENABLE_CPU_ENGINE})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_CPU_ENGINE_SUPPORT)
endif()

if(${THORVG_ENABLE_GL_ENGINE})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_GL_ENGINE_SUPPORT)
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_GL_TARGET_GLES)
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_GL_TARGET_GL)
    endif()
endif()

if(${THORVG_USE_EXTERNAL_JPG})
    target_link_libraries(${PROJECT_NAME} PRIVATE libjpeg-turbo::turbojpeg)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_EXTERNAL_JPG_LOADER_SUPPORT)
endif()

if(${THORVG_USE_EXTERNAL_PNG})
    target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_EXTERNAL_PNG_LOADER_SUPPORT)
endif()

if(${THORVG_USE_EXTERNAL_WEBP})
    target_link_libraries(${PROJECT_NAME} PRIVATE WebP::webp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_EXTERNAL_WEBP_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_THREADS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_THREAD_SUPPORT)
endif()

if(${THORVG_ENABLE_OPENMP})
    find_package(OpenMP COMPONENTS CXX)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_OPENMP_SUPPORT)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        find_path(OMP_INCLUDE_DIR omp.h
                HINTS /opt/homebrew/opt/libomp/include /usr/local/opt/libomp/include)
        find_library(OMP_LIBRARY omp
                HINTS /opt/homebrew/opt/libomp/lib /usr/local/opt/libomp/lib)
        if(OMP_INCLUDE_DIR AND OMP_LIBRARY)
            target_compile_options(${PROJECT_NAME} PRIVATE -Xpreprocessor -fopenmp)
            target_include_directories(${PROJECT_NAME} PRIVATE ${OMP_INCLUDE_DIR})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${OMP_LIBRARY})
            target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_OPENMP_SUPPORT)
            message(STATUS "OpenMP enabled via Homebrew libomp: ${OMP_LIBRARY}")
        else()
            message(STATUS "OpenMP: Homebrew libomp not found. "
                "Install with 'brew install libomp' to enable. "
                "Falling back to single-threaded post-effects.")
        endif()
    else()
        message(STATUS "OpenMP not found; falling back to single-threaded post-effects.")
    endif()
endif()

if(${THORVG_ENABLE_PARTIAL_RENDER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_PARTIAL_RENDER_SUPPORT)
endif()

if(${THORVG_ENABLE_JPG_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_JPG_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_PNG_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_PNG_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_WEBP_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_WEBP_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_LOTTIE_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_LOTTIE_LOADER_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_LOTTIE_EXPRESSIONS_SUPPORT)
endif()

if(${THORVG_ENABLE_SVG_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_SVG_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_MEDIA_LOADER})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework AVFoundation"
        "-framework CoreVideo"
        "-framework CoreMedia"
        "-framework Foundation"
        "-framework CoreFoundation"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_MEDIA_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_TTF_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_SFNT_LOADER_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_TTF_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_OTF_LOADER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_SFNT_LOADER_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_OTF_LOADER_SUPPORT)
endif()

if(${THORVG_ENABLE_GIF_SAVER})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_GIF_SAVER_SUPPORT)
endif()

if(${THORVG_ENABLE_FILE_IO})
    target_compile_definitions(${PROJECT_NAME} PRIVATE THORVG_FILE_IO_SUPPORT)
endif()

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------
# Layout follows the meson build: headers go to include/thorvg-<major>/,
# the library to lib/ (dll to bin/), plus a pkg-config file. A CMake package
# (find_package(thorvg) -> thorvg::thorvg) is installed as well.

# Library artifacts: runtime (dll) -> bin/, link artifacts (implib/static) -> lib/.
# Object libraries have no link artifacts; install the object files instead.
if(THORVG_LIBRARY_TYPE STREQUAL "OBJECT")
    install(TARGETS ${PROJECT_NAME} EXPORT thorvgTargets
        OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
    )
else()
    install(TARGETS ${PROJECT_NAME} EXPORT thorvgTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# Shared library versioning (libthorvg.so.<major>.<minor>.<patch> on Unix,
# with the .so.<major> symlink; ignored on Windows).
if(THORVG_BUILD_SHARED_LIBRARY)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Public headers. thorvg.h always; the loader-specific API headers only when
# their module is built (they are self-contained, flat include works).
set(THORVG_INSTALL_HEADERS ${CMAKE_SOURCE_DIR}/inc/thorvg.h)
if(${THORVG_ENABLE_LOTTIE_LOADER})
    list(APPEND THORVG_INSTALL_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/lottie/thorvg_lottie.h
    )
endif()
if(${THORVG_ENABLE_MEDIA_LOADER})
    list(APPEND THORVG_INSTALL_HEADERS
        ${CMAKE_SOURCE_DIR}/src/loaders/media/thorvg_media.h
    )
endif()
install(FILES ${THORVG_INSTALL_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# CMake package export: find_package(thorvg) + thorvg::thorvg
install(EXPORT thorvgTargets
    FILE thorvgTargets.cmake
    NAMESPACE thorvg::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Static builds carry the engine/loader dependencies in the target interface
# ($<LINK_ONLY:...>), so consumers of a static thorvg must find them first.
set(THORVG_CONFIG_FIND_DEPENDENCIES "")
if(NOT THORVG_BUILD_SHARED_LIBRARY)
    if(THORVG_ENABLE_OPENMP AND OpenMP_CXX_FOUND)
        string(APPEND THORVG_CONFIG_FIND_DEPENDENCIES "find_dependency(OpenMP)\n")
    endif()
    if(THORVG_USE_EXTERNAL_JPG)
        string(APPEND THORVG_CONFIG_FIND_DEPENDENCIES "find_dependency(libjpeg-turbo)\n")
    endif()
    if(THORVG_USE_EXTERNAL_PNG)
        string(APPEND THORVG_CONFIG_FIND_DEPENDENCIES "find_dependency(PNG)\n")
    endif()
    if(THORVG_USE_EXTERNAL_WEBP)
        string(APPEND THORVG_CONFIG_FIND_DEPENDENCIES "find_dependency(WebP)\n")
    endif()
endif()

file(WRITE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
"# Generated by thorvg's CMakeLists.txt -- do not edit.
include(CMakeFindDependencyMacro)
${THORVG_CONFIG_FIND_DEPENDENCIES}if(NOT TARGET thorvg::${PROJECT_NAME})
    include(\"\${CMAKE_CURRENT_LIST_DIR}/thorvgTargets.cmake\")
endif()
")
install(FILES
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# pkg-config file (thorvg.pc; the meson build installs a versioned
# thorvg-<major>.pc instead).
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(THORVG_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
    set(THORVG_PC_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(THORVG_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(THORVG_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
"prefix=\${pcfiledir}/../..
libdir=${THORVG_PC_LIBDIR}
includedir=${THORVG_PC_INCLUDEDIR}

Name: libthorvg
Description: A Thor library for rendering vector graphics
Version: ${PROJECT_VERSION}
Libs: -L\${libdir} -l${PROJECT_NAME}
Cflags: -I\${includedir}/${PROJECT_NAME}
")
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)