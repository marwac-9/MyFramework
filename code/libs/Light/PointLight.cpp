#include "PointLight.h"
#include <algorithm>
#include "Object.h"
#include "Material.h"
#include "Texture.h"
#include "FBOManager.h"
#include "FrameBuffer.h"



PointLight::PointLight()
{
	shadowMapBuffer = nullptr;
}

PointLight::~PointLight()
{
}

void PointLight::Update()
{
	if (CanCastShadow())
	{
		ProjectionMatrix = Matrix4::OpenGLPersp(fov, shadowMapTexture->aspect, near, object->bounds->radius);
		LightMatrixesVP[0] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3( 1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_POSITIVE_X
		LightMatrixesVP[1] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3(-1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_NEGATIVE_X
		LightMatrixesVP[2] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3( 0.0, 1.0, 0.0), Vector3(0.0,  0.0, 1.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_POSITIVE_Y
		LightMatrixesVP[3] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3( 0.0,-1.0, 0.0), Vector3(0.0,  0.0,-1.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
		LightMatrixesVP[4] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3( 0.0, 0.0, 1.0), Vector3(0.0, -1.0, 0.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_POSITIVE_Z
		LightMatrixesVP[5] = Matrix4::lookAt(object->node->GetWorldPosition(), object->node->GetWorldPosition() + Vector3( 0.0, 0.0,-1.0), Vector3(0.0, -1.0, 0.0)) * ProjectionMatrix; //GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	}
}

void PointLight::UpdateScale()
{
	//bizzare size results
	//we could calculate the radius from the light attenuation but it's more convinient to set scale ourselves and in shader always fade out to the radius
	float MaxChannel = 0.5f;// fmax(fmax(object->mat->color.x, object->mat->color.y), object->mat->color.z);
	
	float ret = (-attenuation.Linear + sqrtf(attenuation.Linear * attenuation.Linear - 4 * attenuation.Exponential * (attenuation.Constant - (256.0 / 5) * MaxChannel))) / (2 * attenuation.Exponential);

	//float ret = (-attenuation.Linear + sqrtf(attenuation.Linear * attenuation.Linear - 4 * attenuation.Exponential * (attenuation.Constant - 256 * MaxChannel * object->mat->diffuseIntensity))) / (2 * attenuation.Exponential);

	object->node->SetScale(Vector3(ret, ret, ret));
}

FrameBuffer * PointLight::GenerateShadowMapBuffer(int width, int height)
{
	if (shadowMapBuffer == nullptr)
	{
		LightMatrixesVP.resize(6);
		shadowMapBuffer = FBOManager::Instance()->Generate3DShadowMapBuffer(width, height);
		shadowMapTexture = shadowMapBuffer->textures[0];
	}
	return shadowMapBuffer;
}

void PointLight::DeleteShadowMapBuffer()
{
	if (shadowMapBuffer != nullptr)
	{
		shadowMapBuffer->DeleteAllTextures();
		FBOManager::Instance()->DeleteFrameBuffer(shadowMapBuffer);
		shadowMapBuffer = nullptr;
	}
}

void PointLight::ResizeShadowMap(int width, int height)
{
	shadowMapBuffer->UpdateTextures(width, height);
}

bool PointLight::CanCastShadow()
{
	return shadowMapBuffer != nullptr && shadowMapActive;
}

bool PointLight::CanBlurShadowMap()
{
	return blurMode != BlurMode::None && shadowMapBlurActive;
}