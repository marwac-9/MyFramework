#pragma once
#include <GL/glew.h>
#include <vector>
#include "MyMathLib.h"
#include "BoundingBox.h"
#include "Node.h"
#include "OBBAABB.h"
class Material;
class Mesh;

class Object
{
public:
	Object();
	~Object();
	Node node;
	Material* mat;
	Mesh* mesh;
	int ID;
	float radius;
	mwm::Vector3 boundingSphereOffset;
	void AssignMaterial(Material* mat);
	void AssignMesh(Mesh* mesh);
	int indicesSize;
	void draw(const mwm::Matrix4& Projection, const mwm::Matrix4& View);
	void drawLight(const mwm::Matrix4& Projection, const mwm::Matrix4& View, const mwm::Vector3& camPos);
	void drawGeometry(const mwm::Matrix4& Projection, const mwm::Matrix4& View);
	mwm::Vector3 extractScale();
	mwm::Vector3 getScale();
	
	void setRadius(float radius);	
	
	void IntegrateEuler(float timestep, const mwm::Vector3& gravity);
	void IntegrateMid(float timestep, const mwm::Vector3& gravity);
	void IntegrateMid2(float timestep, const mwm::Vector3& gravity);
	void IntegrateRunge2(float timestep, const mwm::Vector3& gravity); //acceleration only
	void IntegrateRunge3(float timestep, const mwm::Vector3& gravity); //acceleration pos and rot
	void setAwake(const bool awake = true);
	void setCanSleep(const bool canSleep);
	void ApplyImpulse(const mwm::Vector3& force, const mwm::Vector3& picking_point);
	void ApplyImpulse(const mwm::Vector3& direction, float magnitude, const mwm::Vector3& point);
	float GetMass();
	float GetMassInverse();
	void SetMass(float mass);
	mwm::Vector3 GetMeshDimensions();
	void SetInertiaTensor(const mwm::Matrix3& I);
	
	void SetPosition(float x, float y, float z);
	void SetPosition(const mwm::Vector3& vector);
	mwm::Vector3 GetPosition() const;
	
	void SetScale(float x, float y, float z);
	void SetScale(const mwm::Vector3& vector);
	
	void Translate(float x, float y, float z);
	void Translate(const mwm::Vector3& vector);
	
	void SetOrientation(const mwm::Quaternion& q);
	mwm::Quaternion GetOrientation();
	
	float mass = 1.f;
	float massInverse = 1.f/1.f;
	float damping = 0.85f;
	mwm::Vector3 accum_force;
	mwm::Vector3 accum_torque;
	mwm::Matrix3 inverse_inertia_tensor;
	mwm::Vector3 velocity;
	mwm::Vector3 angular_velocity;
	
	mwm::Vector3 acceleration;
	mwm::Vector3 angular_acc;
	void SetMeshOffset(const mwm::Vector3& offset);
	mwm::Vector3 ConvertPointToWorld(const mwm::Vector3& point, const mwm::Matrix4& modelTransform);

	//AABB aabb;
	//OBB obb; //need either aabb or obb as bounding box because aabb is based on obb so their min and max values are the same

	AABB aabb;
	OBB obb;
	void UpdateKineticEnergyStoreAndPutToSleep(float timestep);
	void UpdateBoundingBoxes(const BoundingBox& boundingBox);
	void SetOBBHalfExtent(const mwm::Vector3& scale);
	void UpdateInertiaTensor();
	Object* boundingBoxShape;
	bool isAwake = true;
	mwm::Matrix3 inverse_inertia_tensor_world;
	bool isKinematic = false;
	float restitution = 0.f;

	void UpdatePosAndOrient(float timeStep);
	void drawDepth(const mwm::Matrix4& Projection, const mwm::Matrix4& View);
private:
	mwm::Vector3 meshOffset;
	//should not be here
	GLuint MatrixHandle;
	GLuint ViewMatrixHandle;
	GLuint ModelMatrixHandle;
	GLuint MaterialAmbientIntensityValueHandle;
	GLuint MaterialSpecularIntensityHandle;
	GLuint MaterialDiffuseIntensityValueHandle;
	GLuint MaterialColorHandle;
	GLuint MaterialShininessValue;
	GLuint PickingObjectIndexHandle;
	GLuint TextureSamplerHandle;
	GLuint DepthMVPMatrixHandle;
	GLuint DepthBiasMatrixHandle;
	float motion = 1.f; //make sure it does not sleep directly at start of simulation
	bool canSleep = true;
	float sleepEpsilon = 0.2f; 
	
	mwm::Matrix4 depthMVP = mwm::Matrix4::identityMatrix();
};

