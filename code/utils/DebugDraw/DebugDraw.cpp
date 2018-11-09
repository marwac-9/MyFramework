#pragma once
#include <GL/glew.h>
#include "DebugDraw.h"
#include "Object.h"
#include "Node.h"
#include "GraphicsStorage.h"
#include "Material.h"
#include "Mesh.h"
#include "ShaderManager.h"
#include "Render.h"
using namespace mwm;

DebugDraw::DebugDraw()
{
}

DebugDraw::~DebugDraw()
{
}

DebugDraw* DebugDraw::Instance()
{
	static DebugDraw instance;

	return &instance;
}

void DebugDraw::LoadPrimitives()
{
	debugMat = new Material();
	debugMat->AssignTexture(GraphicsStorage::textures.at(0));

	Object* newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["tetra"]);
	debugShapes["tetra"] = newObject;

	newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["pyramid"]);
	debugShapes["pyramid"] = newObject;

	newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["cube"]);
	debugShapes["cube"] = newObject;

	newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["sphere"]);
	debugShapes["sphere"] = newObject;

	newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["icosphere"]);
	debugShapes["icosphere"] = newObject;

	newObject = new Object();
	newObject->AssignMaterial(debugMat);
	newObject->AssignMesh(GraphicsStorage::meshes["unitCube"]);
	debugShapes["unitCube"] = newObject;
}

void DebugDraw::DrawShapeAtPos(const char* shapeName, const Vector3& pos)
{
	Object* shape = DebugDraw::Instance()->debugShapes[shapeName];
	shape->SetPosition(pos);
	shape->SetScale(Vector3(0.5f, 0.5f, 0.5f));
	shape->node.UpdateNodeTransform(Node());
	Render::Instance()->drawSingle(shape, *View**Projection, ShaderManager::Instance()->GetCurrentShaderID());
}

void DebugDraw::DrawLine(const Vector3& normal, const Vector3& position, float width)
{
	Matrix4 model = Matrix4::translate(position);
	double length = normal.vectLengt();
	Vector3 normalized = normal.vectNormalize();
	
	Vector3 axis = Vector3(0.f, 0.f, 1.f).crossProd(normalized);
	double tetha = acos(normalized.z);
	if (axis.squareMag() < 0.0001f)
	{
		axis = Vector3(1.f, 0.f, 0.f);
	}
	if (tetha != 0)
	{
		double  deg = (tetha * 180.f) / 3.14159f;
		model = Matrix4::rotateAngle(axis, deg)*model;
	}

	model = Matrix4::scale(Vector3(length, length, length))*model;
	
	
	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	line.Draw(model, *View, *Projection, wireframeShader, width);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}


void DebugDraw::DrawNormal(const Vector3& normal, const Vector3& position, float width /*= 4.f*/)
{
	Matrix4 model = Matrix4::translate(position);
	Vector3 axis = Vector3(0.f, 0.f, 1.f).crossProd(normal);
	double tetha = acos(normal.z);
	if (axis.squareMag() < 0.0001f)
	{
		axis = Vector3(1.f, 0.f, 0.f);
	}
	if (tetha != 0)
	{
		double  deg = (tetha * 180.f) / 3.14159f;
		model = Matrix4::rotateAngle(axis, deg)*model;
	}
	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	line.Draw(model, *View, *Projection, wireframeShader, width);
	point.Draw(model, *View, *Projection, wireframeShader);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}

void DebugDraw::DrawPlane(const Vector3& normal, const Vector3& position, const Vector3& halfExtent)
{
	Matrix4 model = Matrix4::translate(position);
	Vector3 axis = Vector3(0.f,0.f,1.f).crossProd(normal);
	double tetha = acos(normal.z);
	if (axis.squareMag() < 0.0001f)
	{
		axis = Vector3(1.f, 0.f, 0.f);
	}
	if (tetha != 0)
	{
		double  deg = (tetha * 180.f) / 3.14159f;
		model = Matrix4::rotateAngle(axis, deg)*model;
	}
	model = Matrix4::scale(halfExtent) * model;

	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	plane.Draw(model, *View, *Projection, wireframeShader);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}


void DebugDraw::DrawPlaneN(const Vector3& normal, const Vector3& position, const Vector3& halfExtent /*= Vector3(1, 1, 1)*/)
{
	Matrix4 model = Matrix4::translate(position);
	Vector3 axis = Vector3(0.f, 0.f, 1.f).crossProd(normal);
	double tetha = acos(normal.z);
	if (axis.squareMag() < 0.0001f)
	{
		axis = Vector3(1.f, 0.f, 0.f);
	}
	if (tetha != 0)
	{
		double  deg = (tetha * 180.f) / 3.14159f;
		model = Matrix4::rotateAngle(axis, deg)*model;
	}
	model = Matrix4::scale(halfExtent) * model;

	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	plane.Draw(model, *View, *Projection, wireframeShader);
	line.Draw(model, *View, *Projection, wireframeShader);
	point.Draw(model, *View, *Projection, wireframeShader);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}


void DebugDraw::DrawPoint(const Vector3& position, float size)
{
	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	point.Draw(Matrix4::translate(position), *View, *Projection, wireframeShader, size);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}

void DebugDraw::DrawCrossHair(int windowWidth, int windowHeight, const Vector3F& color)
{
	double scale = 20.0;
	double offset = scale / 2.0;
	double x = windowWidth / 2.0;
	double y = windowHeight / 2.0;
	Matrix4 model1 = Matrix4::translate(x, y + offset, 0.0);
	Matrix4 model2 = Matrix4::translate(x - offset, y, 0.0);

	model1 = Matrix4::rotateAngle(Vector3(1.0, 0.0, 0.0), 90.0)*model1;
	model2 = Matrix4::rotateAngle(Vector3(0.0, 1.0, 0.0), 90.0)*model2;
	Matrix4 scaleM = Matrix4::scale(scale, scale, scale);
	model1 = scaleM * model1;
	model2 = scaleM * model2;
	Matrix4 view = Matrix4(1);
	Matrix4 proj = Matrix4::orthographicTopToBottom(-1.0, 2000.0, 0.0, (double)windowWidth, (double)windowHeight, 0.0);
	line.mat->color = color;

	GLuint prevShader = ShaderManager::Instance()->GetCurrentShaderID();
	GLuint wireframeShader = GraphicsStorage::shaderIDs["wireframe"];
	ShaderManager::Instance()->SetCurrentShader(wireframeShader);
	line.Draw(model1, view, proj, wireframeShader, 2.f);
	line.Draw(model2, view, proj, wireframeShader, 2.f);
	ShaderManager::Instance()->SetCurrentShader(prevShader);
}

void DebugDraw::DrawQuad()
{
	//bind vao before drawing
	glBindVertexArray(plane.mesh->vaoHandle);

	// Draw the triangles !
	glDrawElements(GL_TRIANGLES, plane.mesh->indicesSize, GL_UNSIGNED_SHORT, (void*)0); // mode, count, type, element array buffer offset
}

void DebugDraw::DrawMap(int posX, int posY, int width, int height, unsigned int textureHandle, int windowWidth, int windowHeight)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(posX, posY, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	glViewport(posX, posY, width, height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	DrawQuad();

	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(0, 0, windowWidth, windowHeight);
}