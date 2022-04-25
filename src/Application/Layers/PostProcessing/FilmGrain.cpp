#include "FilmGrain.h"

#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "../RenderLayer.h"
#include "Application/Application.h"

filmGrain::filmGrain() :
	PostProcessingLayer::Effect(),
	_shader(nullptr)
{
	Name = "Film Grain";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/Film_Grain.glsl" }
	});
}

filmGrain::~filmGrain() = default;

void filmGrain::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	gBuffer->BindAttachment(RenderTargetAttachment::Depth, 1);
}

void filmGrain::RenderImGui()
{
}

filmGrain::Sptr filmGrain::FromJson(const nlohmann::json & data)
{
	filmGrain::Sptr result = std::make_shared<filmGrain>();
	result->Enabled = JsonGet(data, "enabled", true);
	return result;
}

nlohmann::json filmGrain::ToJson() const
{
	return {
		{ "enabled", Enabled }
	};
}
