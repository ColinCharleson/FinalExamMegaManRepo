#pragma once

#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Framebuffer.h"

class filmGrain : public PostProcessingLayer::Effect {
public:
	MAKE_PTRS(filmGrain);

	filmGrain();
	virtual ~filmGrain();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource

	filmGrain::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;
};