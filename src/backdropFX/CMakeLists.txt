SET( LIB_NAME backdropFX )

ADD_DEFINITIONS( -DBACKDROPFX_LIBRARY )
SET( HEADER_PATH ${PROJECT_SOURCE_DIR}/include/${LIB_NAME} )

SET( LIB_PUBLIC_HEADERS
    ${HEADER_PATH}/BackdropCommon.h
    ${HEADER_PATH}/DepthPartition.h
    ${HEADER_PATH}/DepthPartitionStage.h
    ${HEADER_PATH}/DepthPeelBin.h
    ${HEADER_PATH}/DepthPeelUtils.h
    ${HEADER_PATH}/Effect.h
    ${HEADER_PATH}/EffectLibrary.h
    ${HEADER_PATH}/EffectLibraryUtils.h
    ${HEADER_PATH}/Export.h
    ${HEADER_PATH}/LocationData.h
    ${HEADER_PATH}/Manager.h
    ${HEADER_PATH}/MoonBody.h
    ${HEADER_PATH}/RenderingEffects.h
    ${HEADER_PATH}/RenderingEffectsStage.h
    ${HEADER_PATH}/RTTViewport.h
    ${HEADER_PATH}/ShaderLibraryConstants.h
    ${HEADER_PATH}/ShaderModule.h
    ${HEADER_PATH}/ShaderModuleUtils.h
    ${HEADER_PATH}/ShaderModuleVisitor.h
    ${HEADER_PATH}/SkyDome.h
    ${HEADER_PATH}/SkyDomeStage.h
    ${HEADER_PATH}/Stub.h
    ${HEADER_PATH}/StubStage.h
    ${HEADER_PATH}/SunBody.h
    ${HEADER_PATH}/Utils.h
    ${HEADER_PATH}/Version.h
)

ADD_SHARED_LIBRARY_INTERNAL( ${LIB_NAME}
    ${LIB_PUBLIC_HEADERS}
    BackdropCommon.cpp
    DepthPartition.cpp
    DepthPartitionStage.cpp
    DepthPeelBin.cpp
    DepthPeelUtils.cpp
    Effect.cpp
    EffectLibrary.cpp
    EffectLibraryUtils.cpp
    LocationData.cpp
    Manager.cpp
    MoonBody.cpp
    RenderingEffects.cpp
    RenderingEffectsStage.cpp
    RTTViewport.cpp
    ShaderModule.cpp
    ShaderModuleUtils.cpp
    ShaderModuleVisitor.cpp
    SkyDome.cpp
    SkyDomeStage.cpp
    Stub.cpp
    StubStage.cpp
    SunBody.cpp
    Utils.cpp
    Version.cpp
)

TARGET_LINK_LIBRARIES(
    ${LIB_NAME}
    ${OSG_LIBRARIES}
)

INCLUDE(ModuleInstall OPTIONAL)
