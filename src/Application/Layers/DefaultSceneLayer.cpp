#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/Textures/Texture2DArray.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/EnemyBullet.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
#include "Gameplay/Components/ShipMoveBehaviour.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr humanMesh = ResourceManager::CreateAsset<MeshResource>("character.obj");
		MeshResource::Sptr turretMesh = ResourceManager::CreateAsset<MeshResource>("turret.obj");
		MeshResource::Sptr ballMesh = ResourceManager::CreateAsset<MeshResource>("ball.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/BrickTexture.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    playerTex    = ResourceManager::CreateAsset<Texture2D>("textures/playerTexture.png");
		Texture2D::Sptr    ballTex    = ResourceManager::CreateAsset<Texture2D>("textures/ballTexture.png");
		Texture2D::Sptr    turretTex    = ResourceManager::CreateAsset<Texture2D>("textures/turretTexture.png");
		Texture2D::Sptr    youWinTex	= ResourceManager::CreateAsset<Texture2D>("textures/YouWin.png");
		Texture2D::Sptr    youLoseTex	= ResourceManager::CreateAsset<Texture2D>("textures/YouLose.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		//Texture2D::Sptr    brick = ResourceManager::CreateAsset<Texture2D>("textures/BrickTexture.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles.png", 2, 2);

		//DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr playerMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			playerMaterial->Name = "Player Mat";
			playerMaterial->Set("u_Material.AlbedoMap", playerTex);
			playerMaterial->Set("u_Material.NormalMap", normalMapDefault);
			playerMaterial->Set("s_ToonTerm", toonLut);
			playerMaterial->Set("u_Material.Shininess", 0.1f);
			playerMaterial->Set("u_Material.Steps", 8);
		}
		Material::Sptr ballMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			ballMaterial->Name = "Enemy Mat";
			ballMaterial->Set("u_Material.AlbedoMap", ballTex);
			ballMaterial->Set("u_Material.EmissiveMap", ballTex);
			ballMaterial->Set("u_Material.NormalMap", normalMapDefault);
			ballMaterial->Set("u_Material.Shininess", 0.8f);
		}
		Material::Sptr turretMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			turretMaterial->Name = "Enemy Mat";
			turretMaterial->Set("u_Material.AlbedoMap", turretTex);
			turretMaterial->Set("u_Material.NormalMap", normalMapDefault);
			turretMaterial->Set("u_Material.Shininess", 0.3f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon"; 
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f); 
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr polka = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			polka->Name = "Polka";
			polka->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
			polka->Set("u_Material.Specular", solidBlackTex);
			polka->Set("u_Material.NormalMap", normalMapDefault);
			polka->Set("u_Material.EmissiveMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
		}

		Material::Sptr whiteBrick = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			whiteBrick->Name = "White Bricks";
			whiteBrick->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png"));
			whiteBrick->Set("u_Material.Specular", solidGrey);
			whiteBrick->Set("u_Material.NormalMap", ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png"));
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f); 
		}

		// Create some lights for our scene
		GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		for (int ix = 0; ix < 50; ix++) {
			GameObject::Sptr light = scene->CreateGameObject("Light");
			light->SetPostion(glm::vec3(glm::diskRand(25.0f), 1.0f));
			lightParent->AddChild(light);

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)));
			lightComponent->SetRadius(glm::linearRand(0.1f, 10.0f));
			lightComponent->SetIntensity(glm::linearRand(1.0f, 2.0f));
		}

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 0.0f, 0.0f, 3.0f});
			camera->SetRotation({ 70.0f,0.0f, 0.0f });
		} 

		GameObject::Sptr resetWall = scene->CreateGameObject("resetWall");
		{
			MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();

			// Set and rotation position in the scene
			resetWall->SetPostion(glm::vec3(-15.0f, 6.0f, 1.0f));
			resetWall->SetScale(glm::vec3(2.0f, 2.0f, 200.0f));

			RigidBody::Sptr physics = resetWall->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create(glm::vec3(1.0f, 1.0f, 200.0f)))->SetPosition({ 0,0,0 });

			// Add a render component
			RenderComponent::Sptr renderer = resetWall->Add<RenderComponent>();
			renderer->SetMesh(boxMesh);
			renderer->SetMaterial(boxMaterial);
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		GameObject::Sptr player = scene->CreateGameObject("Player");
		{
			// Set position in the scene
			player->SetPostion(glm::vec3(-10.0f, 6.0f, 1.0f));
			player->SetRotation(glm::vec3(0.0f, 0.0f, 180.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = player->Add<RenderComponent>();
			renderer->SetMesh(humanMesh);
			renderer->SetMaterial(playerMaterial);

			RigidBody::Sptr physics = player->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.5f, 0.5f, 1.0f)))->SetPosition({ 0,0,0 });

			TriggerVolume::Sptr triggerVolume = player->Add<TriggerVolume>();
			triggerVolume->AddCollider(BoxCollider::Create(glm::vec3(0.5f, 0.5f, 1.0f)))->SetPosition({ 0,0,0 });
			triggerVolume->SetFlags(TriggerTypeFlags::Dynamics);

			// Add some behaviour that relies on the physics body
			player->Add<SimpleCameraControl>();
			player->Add<JumpBehaviour>();


		}
		
		GameObject::Sptr ballBullet = scene->CreateGameObject("Enemy");
		{
			// Set position in the scene
			ballBullet->SetPostion(glm::vec3(8.0f, 6.0f, 5.0f));
			ballBullet->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = ballBullet->Add<RenderComponent>();
			renderer->SetMesh(ballMesh);
			renderer->SetMaterial(ballMaterial);

			TriggerVolume::Sptr triggerVolume = ballBullet->Add<TriggerVolume>();
			triggerVolume->AddCollider(BoxCollider::Create(glm::vec3(0.3f, 0.3f, 0.3f)))->SetPosition({ 0,0,0 });
			triggerVolume->SetFlags(TriggerTypeFlags::Dynamics);

			// Add some behaviour that relies on the physics body
			ballBullet->Add<EnemyBullet>();
		}

		GameObject::Sptr turret = scene->CreateGameObject("turret");
		{
			// Set and rotation position in the scene
			turret->SetPostion(glm::vec3(9.0f, 6.5f, 0.5f));
			turret->SetRotation(glm::vec3(0.0f, 0.0f, -180.0f));
			turret->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// Add a render component
			RenderComponent::Sptr renderer = turret->Add<RenderComponent>();
			renderer->SetMesh(turretMesh);
			renderer->SetMaterial(turretMaterial);

			RigidBody::Sptr physics = turret->Add<RigidBody>();
			physics->AddCollider(BoxCollider::Create(glm::vec3(1.0f, 1.0f, 5.0f)))->SetPosition({ 0,0,0 });
		}

		GameObject::Sptr shadowCaster = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster->SetPostion(glm::vec3(-3.5f, 3.8f, 10.0f));
			shadowCaster->SetRotation(glm::vec3(8.9f, 0.0f, -48.0f));
			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(120.0f), 1.0f, 0.1f, 100.0f));
		}

		/////////////////////////// UI //////////////////////////////
		
		GameObject::Sptr youWin = scene->CreateGameObject("You Win Text");
		{
			RectTransform::Sptr transform = youWin->Add<RectTransform>();
			transform->SetPosition({ 0, 0 });
			transform->SetMin({ 192, 108 });
			transform->SetMax({ 1920, 1080 });

			GuiPanel::Sptr panel = youWin->Add<GuiPanel>();
			panel->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			panel->SetTexture(youWinTex);

			panel->IsEnabled = false;

		}

		GameObject::Sptr youLose = scene->CreateGameObject("You Lose Text");
		{
			RectTransform::Sptr transform = youLose->Add<RectTransform>();
			transform->SetPosition({ 0, 0 });
			transform->SetMin({ 192, 108 });
			transform->SetMax({ 1920, 1080 });

			GuiPanel::Sptr panel = youLose->Add<GuiPanel>();
			panel->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			panel->SetTexture(youLoseTex);

			panel->IsEnabled = false;

		}
		
		{	// Players smoke particle
			ParticleSystem::Sptr particleManager = player->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f, 0.0f, -1.0f);
			emitter.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 0.5f, 0.5f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.1f, 0.5f };

			particleManager->AddEmitter(emitter);
		}

		{	// Enemy smoke particle
			ParticleSystem::Sptr particleManager = ballBullet->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 0.1f, 0.5f };
			emitter.SphereEmitterData.Radius = 0.3f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.5f };

			particleManager->AddEmitter(emitter);
		}
		
		{	// Turret smoke particle
			ParticleSystem::Sptr particleManager = turret->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 3;
			emitter.Position = glm::vec3(0.0f, 0.0f, 9.4f);
			emitter.Color = glm::vec4(1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 1.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 0.5f, 0.5f };
			emitter.SphereEmitterData.Radius = 0.7f;
			emitter.SphereEmitterData.SizeRange = { 1.1f, 2.5f };

			particleManager->AddEmitter(emitter);
		}
		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
