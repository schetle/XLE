// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#pragma once

#include "SceneEngineUtils.h"

#include "../RenderCore/Metal/Format.h"
#include "../RenderCore/Metal/RenderTargetView.h"
#include "../RenderCore/Metal/ShaderResource.h"
#include "../RenderCore/Metal/InputLayout.h"

#include "../BufferUploads/IBufferUploads.h"

namespace SceneEngine
{
    class MainTargetsBox
    {
    public:
        static const unsigned s_gbufferTextureCount = 3;
        class Desc
        {
        public:
            Desc(   unsigned width, unsigned height, 
                    const FormatStack& diffuseFormat, const FormatStack& normalFormat, 
                    const FormatStack& parametersFormat, const FormatStack& depthFormat,
                    const BufferUploads::TextureSamples& sampling);

            unsigned _width, _height;

            FormatStack _gbufferFormats[s_gbufferTextureCount];
            FormatStack _depthFormat;
            BufferUploads::TextureSamples _sampling;
        };

        MainTargetsBox(const Desc& desc);
        ~MainTargetsBox();

        Desc _desc;
        ResourcePtr  _gbufferTextures[s_gbufferTextureCount];
        ResourcePtr  _msaaDepthBufferTexture;
        ResourcePtr  _secondaryDepthBufferTexture;

        typedef RenderCore::Metal::RenderTargetView RTV;
        typedef RenderCore::Metal::DepthStencilView DSV;
        typedef RenderCore::Metal::ShaderResourceView SRV;

        RTV _gbufferRTVs[s_gbufferTextureCount];

        DSV _msaaDepthBuffer;
        DSV _secondaryDepthBuffer;

        SRV _gbufferRTVsSRV[s_gbufferTextureCount];
        SRV _msaaDepthBufferSRV;
        SRV _secondaryDepthBufferSRV;
    };

    class ForwardTargetsBox
    {
    public:
        class Desc
        {
        public:
            Desc(   unsigned width, unsigned height, 
                    const FormatStack& depthFormat,
                    const BufferUploads::TextureSamples& sampling);

            unsigned _width, _height;
            FormatStack _depthFormat;
            BufferUploads::TextureSamples _sampling;
        };

        ForwardTargetsBox(const Desc& desc);
        ~ForwardTargetsBox();

        Desc _desc;
        ResourcePtr  _msaaDepthBufferTexture;
        ResourcePtr  _secondaryDepthBufferTexture;

        typedef RenderCore::Metal::DepthStencilView DSV;
        typedef RenderCore::Metal::ShaderResourceView SRV;

        DSV _msaaDepthBuffer;
        DSV _secondaryDepthBuffer;

        SRV _msaaDepthBufferSRV;
        SRV _secondaryDepthBufferSRV;
    };

    class LightingResolveTextureBox
    {
    public:
        class Desc
        {
        public:
            Desc(   unsigned width, unsigned height, 
                    const FormatStack& lightingResolveFormat,
                    const BufferUploads::TextureSamples& sampling);

            unsigned _width, _height;
            FormatStack _lightingResolveFormat;
            BufferUploads::TextureSamples _sampling;
        };

        typedef RenderCore::Metal::RenderTargetView RTV;
        typedef RenderCore::Metal::ShaderResourceView SRV;

        ResourcePtr     _lightingResolveTexture;
        RTV             _lightingResolveRTV;
        SRV             _lightingResolveSRV;

        ResourcePtr     _lightingResolveCopy;
        SRV             _lightingResolveCopySRV;

        LightingResolveTextureBox(const Desc& desc);
        ~LightingResolveTextureBox();
    };


    class LightingResolveShaders
    {
    public:
        class Desc
        {
        public:
            unsigned    _msaaSampleCount;
            bool        _msaaSamplers, _flipDirection;
            unsigned    _gbufferType;
            unsigned    _dynamicLinking;
            bool        _debugging;

            Desc(unsigned gbufferType, unsigned msaaSampleCount, bool msaaSamplers, bool flipDirection, unsigned dynamicLinking, bool debugging)
            {
                    //  we have to "memset" this -- because padding adds
                    //  random values in profile mode
                std::fill((char*)this, PtrAdd((char*)this, sizeof(*this)), 0);
                _gbufferType = gbufferType;
                _msaaSampleCount = msaaSampleCount;
                _msaaSamplers = msaaSamplers;
                _flipDirection = flipDirection;
                _dynamicLinking = dynamicLinking;
                _debugging = debugging;
            }
        };

        using ShaderProgram = RenderCore::Metal::ShaderProgram;
        using BoundUniforms = RenderCore::Metal::BoundUniforms;
        using BoundClassInterfaces = RenderCore::Metal::BoundClassInterfaces;

        enum Shape : uint8 { Directional, Sphere, Tube, Rectangle, Disc };
        enum Shadowing : uint8 { NoShadows, PerspectiveShadows, OrthShadows, OrthShadowsNearCascade, OrthHybridShadows };
            
        class LightShaderType
        {
        public:
            Shape       _shape;
            Shadowing   _shadows;
            uint8       _diffuseModel;
            uint8       _shadowResolveModel;
            bool        _hasScreenSpaceAO;

            unsigned AsIndex() const;
            static unsigned ReservedIndexCount();

            LightShaderType(
                Shape shape, Shadowing shadows, 
                uint8 diffuseModel, uint8 shadowResolveModel, bool hasScreenSpaceAO)
                : _shape(shape), _shadows(shadows), _diffuseModel(diffuseModel), _shadowResolveModel(shadowResolveModel)
                , _hasScreenSpaceAO(hasScreenSpaceAO) {}
            LightShaderType()
                : _shape(Directional), _shadows(NoShadows), _diffuseModel(0), _shadowResolveModel(0), _hasScreenSpaceAO(false) {}
        };

        class LightShader
        {
        public:
            const ShaderProgram*    _shader;
            BoundUniforms           _uniforms;
            BoundClassInterfaces    _boundClassInterfaces;
            bool                    _dynamicLinking;

            LightShader() : _shader(nullptr), _dynamicLinking(false) {}
        };

        struct CB
        {
            enum 
            {
                ShadowProj_Arbit,
                LightBuffer,
                ShadowParam,
                ScreenToShadow,
                ShadowProj_Ortho,
                ShadowResolveParam,
                ScreenToRTShadow,
                Debugging,
                Max
            };
        };

        struct SR
        {
            enum
            {
                RTShadow_ListHead,
                RTShadow_LinkedLists,
                RTShadow_Triangles,
                Max
            };
        };

        const LightShader* GetShader(const LightShaderType& type);

        const ::Assets::DepValPtr& GetDependencyValidation() const   { return _validationCallback; }

        LightingResolveShaders(const Desc& desc);
        ~LightingResolveShaders();
    private:
        ::Assets::DepValPtr _validationCallback;
        std::vector<LightShader> _shaders;

        void BuildShader(const Desc& desc, const LightShaderType& type);
    };


    class AmbientResolveShaders
    {
    public:
        class Desc
        {
        public:
            unsigned _msaaSampleCount;
            bool _msaaSamplers;
            bool _flipDirection;
            bool _hasAO, _hasTiledLighting, _hasSRR;
            bool _rangeFog;
            unsigned _skyProjectionType;
            bool _hasIBL;
            unsigned _gbufferType;
            bool _referenceShaders;

            Desc(   unsigned gbufferType,
                    unsigned msaaSampleCount, bool msaaSamplers, bool flipDirection,
                    bool hasAO, bool hasTiledLighting, bool hasSSR, 
                    unsigned skyProjectionType, bool hasIBL,
                    bool rangeFog, bool referenceShaders)
            {
                    //  we have to "memset" this -- because padding adds random values in 
                    //  profile mode
                std::fill((char*)this, PtrAdd((char*)this, sizeof(*this)), 0);
                _gbufferType = gbufferType;
                _msaaSampleCount = msaaSampleCount;
                _msaaSamplers = msaaSamplers;
                _flipDirection = flipDirection;
                _hasAO = hasAO; _hasSRR = hasSSR; _hasTiledLighting = hasTiledLighting;
                _skyProjectionType = skyProjectionType;
                _hasIBL = hasIBL;
                _rangeFog = rangeFog;
                _referenceShaders = referenceShaders;
            }
        };

        const RenderCore::Metal::ShaderProgram*             _ambientLight;
        const RenderCore::Metal::ShaderProgram*             _ambientLightRangeFog;
        std::unique_ptr<RenderCore::Metal::BoundUniforms>   _ambientLightUniforms;

        const std::shared_ptr<::Assets::DependencyValidation>& GetDependencyValidation() const   { return _validationCallback; }

        AmbientResolveShaders(const Desc& desc);
        ~AmbientResolveShaders();

    private:
        std::shared_ptr<::Assets::DependencyValidation>  _validationCallback;
    };


 
    #if defined(_DEBUG)
        void SaveGBuffer(RenderCore::Metal::DeviceContext& context, MainTargetsBox& mainTargets);
    #endif

    class LightingParserContext;
    void Deferred_DrawDebugging(RenderCore::Metal::DeviceContext& context, LightingParserContext& parserContext, MainTargetsBox& mainTargets, unsigned debuggingType);

}

