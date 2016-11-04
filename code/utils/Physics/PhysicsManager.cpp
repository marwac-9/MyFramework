#include "PhysicsManager.h"
#include "Material.h"
#include "Node.h"
#include "DebugDraw.h"
#include <algorithm>

using namespace mwm;
using namespace std;

PhysicsManager::PhysicsManager()
{
	defObbColor = Vector3(0.f, 0.8f, 0.8f);
	defAabbColor = Vector3(1.f, 0.54f, 0.f);
}

//this is process for face contacts using vector<Vector3> contacts
void PhysicsManager::ProcessContact(Contact* contact, Vector3&vel1, Vector3& ang_vel1, Vector3&vel2, Vector3& ang_vel2, float dtInv)
{
	const float k_allowedPenetration = 0.01f;
	float BAUMGARTE = 0.2f;
	//BAUMGARTE
	float bias = -BAUMGARTE * dtInv * std::min(0.0f, -contact->penetration + k_allowedPenetration);

	//compute impulse force
	Object* ent1 = contact->one;
	Object* ent2 = contact->two;

	const float e = std::min(ent1->restitution, ent2->restitution);//0.0f; //restitution, if set to 0 then the object we collide with will absorb most of the impact and won't move much, if set to 1 both objects will fly their way
	Vector3 contactPoint = contact->contactPoint;
	Vector3 contactNormal = contact->contactNormal;
	Vector3 rA = contactPoint - ent1->node.position;
	Vector3 rB = contactPoint - ent2->node.position;
	Vector3 kA = rA.crossProd(contactNormal);
	Vector3 kB = rB.crossProd(contactNormal);

	Vector3 uA = ent1->inverse_inertia_tensor_world*kA;
	Vector3 uB = ent2->inverse_inertia_tensor_world*kB;

	// Precompute normal mass, tangent mass, and bias.

	// Normal Mass //is it force of normal pushing away from the surface?
	float normalMass = ent1->massInverse + ent2->massInverse;

	//if (!ent1->isKinematic)	{
		normalMass += kA.dotAKAscalar(uA);
	//}
	//if (!ent2->isKinematic)	{
		normalMass += kB.dotAKAscalar(uB);
	//}

	//problem lies in the kinematic object to have zero inertia tensor
	//relative velocity then is not changed
	//but the denominator or normal mass will be too low 

	// Relative velocity at contact
	Vector3 relativeVel = (ent2->velocity + ent2->angular_velocity.crossProd(rB)) - (ent1->velocity + ent1->angular_velocity.crossProd(rA));
	float numer = -(1.f + e)*relativeVel.dotAKAscalar(contactNormal) +bias;
	//double denom = ent1->massInverse + ent2->massInverse + (ent1->inverse_inertia_tensor_world*kA.crossProd(rA) + ent2->inverse_inertia_tensor_world*kB.crossProd(rB)).dotAKAscalar(contactNormal);
	//double f = numer / denom;
	float f = numer / normalMass;
	f = std::max(f, 0.0f);

	Vector3 impulse = f*contactNormal;
	//printf("\nimpulse: %f", f);
	
	vel1 -= impulse*ent1->massInverse;
	ang_vel1 -= f*uA;

	vel2 += impulse*ent2->massInverse;
	ang_vel2 += f*uB;
	

	if (DebugDraw::Instance()->debug)
	{
		//impuls normal
		DebugDraw::Instance()->line.mat->SetColor(0.f, 1.f, 1.f);
		DebugDraw::Instance()->point.mat->SetColor(0.f, 1.f, 1.f);
		DebugDraw::Instance()->DrawNormal(contactNormal, ent2->node.position, 6.f); //since it's ent2 which gets +impulse
	}
}

PhysicsManager::~PhysicsManager()
{
}

PhysicsManager* PhysicsManager::Instance()
{
	static PhysicsManager instance;

	return &instance;
}

void PhysicsManager::SortAxis(vector<ObjectPoint*>& axisList, axis axis)
{

	//Each Object contains min and max values
	//Each Object is added to the list two times and one of them is marked as min and the other one as max
	//each time we get value we test if the we want to get it from max or min vectors
	//We want the min and max values to be updated from objects and we don't want our sort and sweep to affect them or their order
	//therefore we store pointers to the objects that contain updated values
	//and so we sort the structs with pointers istead but use the values of objects min and max to sort
	
	
	//we want our fullOverlaps hash set to contain only
	for (size_t j = 1; j < axisList.size(); j++)
	{
		ObjectPoint* currentObject = axisList[j];
		float currentObjectValue = currentObject->value(axis);

		int i = j - 1;
		while (i >= 0 && axisList[i]->value(axis) > currentObjectValue) // we check if values ale bigger or smaller
		{
			ObjectPoint* objToSwap = axisList[i];

			if (currentObject->isMin && !objToSwap->isMin) //penetration
			{
				if (!objToSwap->obj->isKinematic || !currentObject->obj->isKinematic)
				{
					if (CheckBoundingBoxes(objToSwap->obj, currentObject->obj))
					{
						fullOverlaps.insert(OverlapPair(objToSwap->obj, currentObject->obj));
					}
				}
			}

			else if (!currentObject->isMin && objToSwap->isMin) //separation
			{
				fullOverlaps.erase(OverlapPair(objToSwap->obj, currentObject->obj));
				satOverlaps.erase(OverlapPair(objToSwap->obj, currentObject->obj));
				currentObject->obj->aabb.color = defAabbColor;
				objToSwap->obj->aabb.color = defAabbColor;
			}

			axisList[i + 1] = objToSwap;
			i = i - 1;
		}
		axisList[i + 1] = currentObject;
	}
}

bool PhysicsManager::CheckBoundingBoxes(Object* obj1, Object* obj2)
{
	MinMax box1 = obj1->obb.mm;
	MinMax box2 = obj2->obb.mm;

	return ((
		((box1.max.z >= box2.min.z) && (box1.min.z <= box2.max.z)) &&
		((box1.max.y >= box2.min.y) && (box1.min.y <= box2.max.y))) &&
		((box1.max.x >= box2.min.x) && (box1.min.x <= box2.max.x)));
}

void PhysicsManager::AddObject(Object* obj)
{
	ObjectPoint* objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = true;
	xAxis.push_back(objectPoint); //min

	objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = false;
	xAxis.push_back(objectPoint); //max


	objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = true;
	yAxis.push_back(objectPoint); 

	objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = false;
	yAxis.push_back(objectPoint);


	objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = true;
	zAxis.push_back(objectPoint); 

	objectPoint = new ObjectPoint();
	objectPoint->obj = obj;
	objectPoint->isMin = false;
	zAxis.push_back(objectPoint);
}

void  PhysicsManager::SortAndSweep()
{
	SortAxis(xAxis, x);
	SortAxis(yAxis, y);
	SortAxis(zAxis, z);
	//printf("\nBroad: number of overlaps: %d", fullOverlaps.size());
}

#define TEST_OVERLAP(axis, index) overlapOnAxis(oneObj, twoObj, (axis), (index), axisNumRes, toCentre, smallestPenetration, smallestAxis)
bool PhysicsManager::IntersectionTest(const Object* oneObj, const Object* twoObj, float& smallestPenetration, Vector3& smallestAxis, Vector3& toCentre, int& axisNumRes, int& bestSingleAxis)
{
	smallestPenetration = FLT_MAX;
	axisNumRes = 255;
	// Find the vector between the two centres
	toCentre = twoObj->node.position - oneObj->node.position;

	Vector3 oneRight = oneObj->obb.model.getAxis(0);
	Vector3 oneUp = oneObj->obb.model.getAxis(1);
	Vector3 oneBack = oneObj->obb.model.getAxis(2);

	Vector3 twoRight = twoObj->obb.model.getAxis(0);
	Vector3 twoUp = twoObj->obb.model.getAxis(1);
	Vector3 twoBack = twoObj->obb.model.getAxis(2);

	// Check on box one's axes first
	if (TEST_OVERLAP(oneRight, 0) &&
		TEST_OVERLAP(oneUp, 1) &&
		TEST_OVERLAP(oneBack, 2) &&
		// And on two's
		TEST_OVERLAP(twoRight, 3) &&
		TEST_OVERLAP(twoUp, 4) &&
		TEST_OVERLAP(twoBack, 5))
	{
		bestSingleAxis = axisNumRes;
		if (// Now on the cross products
			TEST_OVERLAP(oneRight.crossProd(twoRight), 6) &&
			TEST_OVERLAP(oneRight.crossProd(twoUp), 7) &&
			TEST_OVERLAP(oneRight.crossProd(twoBack), 8) &&

			TEST_OVERLAP(oneUp.crossProd(twoRight), 9) &&
			TEST_OVERLAP(oneUp.crossProd(twoUp), 10) &&
			TEST_OVERLAP(oneUp.crossProd(twoBack), 11) &&

			TEST_OVERLAP(oneBack.crossProd(twoRight), 12) &&
			TEST_OVERLAP(oneBack.crossProd(twoUp), 13) &&
			TEST_OVERLAP(oneBack.crossProd(twoBack), 14)
			)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}	
}
#undef TEST_OVERLAP

void PhysicsManager::GenerateContacts(Vector3& MTV, const float& penetration, Vector3& toCentre, unordered_set<Contact*> &contacts, Object* oneObj, Object* twoObj, int axisNumRes, int& bestSingleAxis)
{
	if (axisNumRes < 3)
	{
		Vector3 reference_face[4];
		Vector3 incident_face[4];
		//GenerateContactPointToFace2(-1.f*toCentre, contacts, MTV, penetration, twoObj, oneObj, axisNumRes, reference_face, incident_face);
		//if (contacts.size() < 3) //bad or no contact points generated
		//{
		//	contacts.clear();
			GenerateContactPointToFace2(toCentre, contacts, MTV, penetration, oneObj, twoObj, axisNumRes, reference_face, incident_face);
		//}
		//printf("\n contacts: %d", contacts.size());
		if (DebugDraw::Instance()->debug) DrawFaceDebug(contacts, reference_face, incident_face, axisNumRes);
	}
	else if (axisNumRes < 6)
	{
		Vector3 reference_face[4];
		Vector3 incident_face[4];
		GenerateContactPointToFace2(-1.f*toCentre, contacts, MTV, penetration, twoObj, oneObj, axisNumRes - 3, reference_face, incident_face);
		if (DebugDraw::Instance()->debug) DrawFaceDebug(contacts, reference_face, incident_face, axisNumRes - 3);
	}
	else
	{
		GenerateContactEdgeToEdge(toCentre, contacts, MTV, penetration, oneObj, twoObj, axisNumRes, bestSingleAxis);
	}
}

void PhysicsManager::NarrowTestSAT(float dtInv)
{
	//since broad phase is first and sat is dependent on it
	//if i remove something from my fullOverlaps in broad phase 
	//next time sat will skip to check that separation 

	//also there are cases where aabb is still colliding but obb not
	//therefore i need to handle removal of the non colliding obbs by sat here too
	std::unordered_set<OverlapPair>::iterator it;
	//printf("\nnumber of AABB overlaps: %d", fullOverlaps.size());
	for (it = fullOverlaps.begin(); it != fullOverlaps.end(); it++)
	{
		auto pair = *it;
		Vector3 MTV;
		float smallestPen = FLT_MAX;
		Vector3 toCentre;
		int axisNumRes = -1;
		int bestSingleAxis;
		Vector3 oneHalfSize = pair.ent1->obb.halfExtent;
		Vector3 twoHalfSize = pair.ent2->obb.halfExtent;
		if (IntersectionTest(pair.ent1, pair.ent2, smallestPen, MTV, toCentre, axisNumRes, bestSingleAxis))
		{
			if (!pair.ent1->isKinematic && !pair.ent2->isKinematic)
			{
				pair.ent1->setAwake();
				pair.ent2->setAwake();
			}
			
			//generate contacts based on the result
			unordered_set<Contact*> contacts; 
			GenerateContacts(MTV, smallestPen, toCentre, contacts, pair.ent1, pair.ent2, axisNumRes, bestSingleAxis);
			satOverlaps.insert(pair);
			Vector3 changeInVel1;
			Vector3 changeInAng_Vel1;
			Vector3 changeInVel2;
			Vector3 changeInAng_Vel2;
			int i = 0;
			for (auto& contact : contacts)
			{
				if (DebugDraw::Instance()->debug)
				{
					//draw final contact points
					DebugDraw::Instance()->line.mat->SetColor(0, 1, 1);
					DebugDraw::Instance()->point.mat->SetColor(0, 1, 1);
					DebugDraw::Instance()->DrawNormal(contact->contactNormal, contact->contactPoint);
				}
				//if (i < 4)
				//{
					ProcessContact(contact, changeInVel1, changeInAng_Vel1, changeInVel2, changeInAng_Vel2, dtInv);
				//}
				
				i++;
			}
			
			if (contacts.size() > 0)
			{
				Contact* contact = *contacts.begin();
				if (!contact->one->isKinematic)
				{
					contact->one->velocity += changeInVel1;
					contact->one->angular_velocity += changeInAng_Vel1;
				}
				if (!contact->two->isKinematic)
				{
					contact->two->velocity += changeInVel2;
					contact->two->angular_velocity += changeInAng_Vel2;
				}
				PositionalCorrection(contact->one, contact->two, smallestPen, contact->contactNormal);
			}
		}
		else
		{
			pair.ent1->aabb.color = defAabbColor;
			pair.ent2->aabb.color = defAabbColor;
			satOverlaps.erase(pair);
		}
	}
	//printf("\nnumber of SAT overlaps: %d", satOverlaps.size());
}

void PhysicsManager::GenerateContactPointToFace2(
	Vector3 &toCentre,
	unordered_set<Contact*> &contact_points_out,
	Vector3& mtv,
	float smallestPen,
	Object* oneObj, Object* twoObj,
	int typeOfCollision,
	Vector3 * reference_face, Vector3 * incident_face)
{
	Vector3 normal1, normal2;
	float neg_offset1, pos_offset1, neg_offset2, pos_offset2;
	
	CreateSidePlanesOffsetsAndNormals(oneObj->node.position, typeOfCollision, normal1, oneObj->obb.model, normal2, neg_offset1, oneObj->obb.halfExtent, pos_offset1, neg_offset2, pos_offset2);
	
	FlipMTVTest(mtv, toCentre); //this will calculate correct reference normal pointing from box one to box two just like for edge to edge
	Vector3 incident_axis; //correct incident axis is not the mtv it's the most opposite axis to the mtv(in fact reference normal) of the object that collided with face 
	ClaculateIncidentAxis(incident_axis, twoObj->obb.model, mtv);

	//plane is 
	//normal and point on plane
	//all points on plane are described with
	//normal dot all points - normal dot point on plane
	//we can write
	//normal dot (any point - point on plane)
	//or 
	//any point -> B
	//point on plane -> A
	//AB = B-A
	//normal dot AB = 0
	//This expression can be written as normal dot any point equals d
	//where d = normal dot point on plane, d is then interpreted as a signed distance
	//if point is on plane then
	//normal dot any point - d = 0
	//so it's (normal dot any point) - (normal dot point on plane) = 0 if object is on plane, - if behind, + if in front
	//When a normalized plane equation is evaluated for a given point, the
	//obtained result is the signed distance of the point from the plane(negative if the
	//point is behind the plane, otherwise positive).

	//so d is the distance of the plane from the origin if d is described with point on plane
	//we could precompute the plane and use the d in such manner:
	//normal dot any point - d = 0 if the any point is on plane
	//in example case normal is 0 1 0
	//we can assume that point 4 1 2 is on plane
	//d results in 1
	//then we test if any point 0 -3 0 is on plane
	//we get -3 now is it on? below? or above?
	//-3 - 1 = -4 -> below
	//if point is 0 3 0
	//we get 3
	//3 - 1 = 2 -> above plane

	Vector3 normTest(0.f,1.f,0.f);
	Vector3 pointOnPlane(4.f,0.f,2.f); //(4,0,2) has dist 0 while (4,1,2) has dist 1 and (4,-1,2) has -1 dist
	Vector3 pointTest(0.f, -3.f, 0.f);
	float signedDist = normTest.dotAKAscalar(pointTest);
	float signedDist2 = normTest.dotAKAscalar(pointOnPlane);
	
	CalcFaceVertices(twoObj->node.position, incident_face, incident_axis, twoObj->obb.model, twoObj->obb.halfExtent);
	CalcFaceVertices(oneObj->node.position, reference_face, mtv, oneObj->obb.model, oneObj->obb.halfExtent);

	vector<Vector3> clipPolygon;
	size_t vertCount = 0;
	Vector3 normal1Neg = -1.f*normal1;
	Vector3 normal2Neg = -1.f*normal2;

	clipPolygon.push_back(incident_face[0]);
	clipPolygon.push_back(incident_face[1]);
	clipPolygon.push_back(incident_face[2]);
	clipPolygon.push_back(incident_face[3]);

	//incident face is sent then
	vector<Vector3> newClipPolygon = ClipFaceToSidePlane(clipPolygon, normal1Neg, neg_offset1); //red
	//if (DebugDraw::Instance()->debug) vertCount = DrawPlaneClipContacts(contacts, -1.f*normal1, vertCount, Vector3(1, 0, 0));
	newClipPolygon = ClipFaceToSidePlane(newClipPolygon, normal1, pos_offset1); //green
	//if (DebugDraw::Instance()->debug) vertCount = DrawPlaneClipContacts(contacts, normal1, vertCount, Vector3(0, 1, 0));
	newClipPolygon = ClipFaceToSidePlane(newClipPolygon, normal2Neg, neg_offset2); //blue
	//if (DebugDraw::Instance()->debug) vertCount = DrawPlaneClipContacts(contacts, -1.f*normal2, vertCount, Vector3(0, 0, 1));
	newClipPolygon = ClipFaceToSidePlane(newClipPolygon, normal2, pos_offset2); //yellow
	//if (DebugDraw::Instance()->debug) vertCount = DrawPlaneClipContacts(contacts, normal2, vertCount, Vector3(1, 1, 0));

	//ref normal is mtv
	float refPlaneOffset = oneObj->node.position.dotAKAscalar(mtv);
	refPlaneOffset = refPlaneOffset + oneObj->obb.halfExtent[typeOfCollision];

	//slower more logical way
	//Vector3 centerPointOfRefFace = oneObj->node.position + mtv*oneObj->obb.halfExtent[typeOfCollision];
	//float pos_offsettT = centerPointOfRefFace.dotAKAscalar(mtv); 

	FilterContactsAgainstReferenceFace(newClipPolygon, mtv, refPlaneOffset, contact_points_out, smallestPen, oneObj, twoObj);
	//FilterContactsAgainstReferenceFace2(contacts, mtv, refPlaneOffset, normal1Neg, neg_offset1, normal1, pos_offset1, normal2Neg, neg_offset2, normal2, pos_offset2, contact_points_out, smallestPen, oneObj, twoObj);
}

void PhysicsManager::GenerateContactPointToFace(
	Object* oneObj, Object* twoObj,
	const Vector3 &toCentre,
	unordered_set<Contact*> &data,
	int axisNum,
	float smallestPen
	)
{
	// This method is called when we know that a vertex from
	// box two is in contact with box one.

	Contact* contact = new Contact();

	// We know which axis the collision is on (i.e. best),
	// but we need to work out which of the two faces on
	// this axis.
	Vector3 mtv = oneObj->obb.model.getAxis(axisNum);
	float toCentreDist = mtv.dotAKAscalar(toCentre);
	if (toCentreDist > 0) //have turned sign!
	{
		mtv = mtv * -1.0f;
	}

	// Work out which vertex of box two we're colliding with.
	// Using toCentre doesn't work!
	Vector3 vertex = twoObj->obb.halfExtent;
	if (twoObj->obb.model.getAxis(0).dotAKAscalar(mtv) < 0) vertex.x = -vertex.x;
	if (twoObj->obb.model.getAxis(1).dotAKAscalar(mtv) < 0) vertex.y = -vertex.y;
	if (twoObj->obb.model.getAxis(2).dotAKAscalar(mtv) < 0) vertex.z = -vertex.z;

	// Create the contact data
	contact->contactNormal = mtv;
	contact->penetration = smallestPen;
	contact->contactPoint = twoObj->obb.model * vertex + twoObj->node.position;
	contact->one = oneObj;
	contact->two = twoObj;

	data.insert(contact);
}

void PhysicsManager::GenerateContactEdgeToEdge(const Vector3 &toCentre, unordered_set<Contact*> &contact_points_out, Vector3& mtv, float smallestPen, Object* oneObj, Object* twoObj, int axisNumRes, int& bestSingleAxis)
{
	// We've got an edge-edge contact. Find out which axes
	
	axisNumRes -= 6;
	unsigned oneAxisIndex = axisNumRes / 3;
	unsigned twoAxisIndex = axisNumRes % 3;
	Vector3 oneAxis = oneObj->obb.model.getAxis(oneAxisIndex);
	Vector3 twoAxis = twoObj->obb.model.getAxis(twoAxisIndex);

	// The axis should point from box one to box two.
	float toCentreDist = mtv.dotAKAscalar(toCentre);
	if (toCentreDist > 0) //have turned sign!
	{
		mtv = mtv * -1.0f;
	}

	// We have the axes, but not the edges: each axis has 4 edges parallel
	// to it, we need to find which of the 4 for each object. We do
	// that by finding the point in the centre of the edge. We know
	// its component in the direction of the box's collision axis is zero
	// (its a mid-point) and we determine which of the extremes in each
	// of the other axes is closest.
	Vector3 ptOnOneEdge = oneObj->obb.halfExtent;
	Vector3 ptOnTwoEdge = twoObj->obb.halfExtent;
	for (unsigned i = 0; i < 3; i++)
	{
		if (i == oneAxisIndex) ptOnOneEdge[i] = 0;
		else if (oneObj->obb.model.getAxis(i).dotAKAscalar(mtv) > 0) ptOnOneEdge[i] = -ptOnOneEdge[i];

		if (i == twoAxisIndex) ptOnTwoEdge[i] = 0;
		else if (twoObj->obb.model.getAxis(i).dotAKAscalar(mtv) < 0) ptOnTwoEdge[i] = -ptOnTwoEdge[i];
	}

	// Move them into world coordinates (they are already oriented
	// correctly, since they have been derived from the axes).
	ptOnOneEdge = oneObj->obb.model * ptOnOneEdge + oneObj->node.position;
	ptOnTwoEdge = twoObj->obb.model * ptOnTwoEdge + twoObj->node.position;

	// So we have a point and a direction for the colliding edges.
	// We need to find out point of closest approach of the two
	// line-segments.

	Vector3 vertex = contactPoint(
		ptOnOneEdge, oneAxis, oneObj->obb.halfExtent[oneAxisIndex],
		ptOnTwoEdge, twoAxis, twoObj->obb.halfExtent[twoAxisIndex],
		bestSingleAxis > 2
		);

	// We can fill the contact.
	Contact* contact = new Contact();
	contact->penetration = smallestPen;
	contact->contactNormal = mtv;
	contact->contactPoint = vertex;
	contact->one = twoObj;
	contact->two = oneObj;

	contact_points_out.insert(contact);
}

void PhysicsManager::PositionalCorrection(Object* one, Object* two, float penetration, Vector3& normal)
{
	const float percent = 0.2f; // usually 20% to 80%
	const float slop = 0.01f; // usually 0.01 to 0.1
	//Vector3 correction = (std::max(penetration - slop, 0.0f) / (one->massInverse + two->massInverse)) * normal;
	Vector3 correction = (std::max(penetration - slop, 0.0f) / (one->massInverse + two->massInverse)) * percent * normal;
	one->Translate(-1 * one->massInverse * correction);
	two->Translate(two->massInverse * correction);
}

void PhysicsManager::PositionalImpulseCorrection(Object* one, Object* two, Contact* contact)
{
	const float percent = 1.0f; // usually 20% to 80%
	const float slop = 0.01f; // usually 0.01 to 0.1
	//Vector3 correction = (std::max(penetration - slop, 0.0f) / (one->massInverse + two->massInverse)) * normal;
	float mag = (std::max(contact->penetration - slop, 0.0f) / (one->massInverse + two->massInverse)) * percent;
	one->ApplyImpulse(-1.f * contact->contactNormal, one->massInverse * mag, contact->contactPoint);
	two->ApplyImpulse(contact->contactNormal, two->massInverse * mag, contact->contactPoint);
}

void PhysicsManager::CalcFaceVertices(const Vector3& pos, Vector3* vertices, const Vector3& axis, const Matrix3& model, const Vector3& halfExtents, bool counterClockwise)
{
	Vector3 xAxis = model.getAxis(0);
	Vector3 yAxis = model.getAxis(1);
	Vector3 zAxis = model.getAxis(2);
	Vector3 referencePos;
	Vector3 u;
	Vector3 v;

	float sign = 0;
	if (abs(sign = axis.dotAKAscalar(xAxis)) > 0.9f)
	{
		if (sign > 0.f) referencePos = pos + halfExtents.x*xAxis;
		else referencePos = pos - halfExtents.x*xAxis;
		u = halfExtents.y*yAxis;
		v = halfExtents.z*zAxis;
	}
	else if (abs(sign = axis.dotAKAscalar(yAxis)) > 0.9f)
	{
		if (sign > 0.f) referencePos = pos + halfExtents.y*yAxis;
		else referencePos = pos - halfExtents.y*yAxis;
		u = halfExtents.x*xAxis;
		v = halfExtents.z*zAxis;
	}
	else if (abs(sign = axis.dotAKAscalar(zAxis)) > 0.9f)
	{
		if (sign > 0.f)	referencePos = pos + halfExtents.z*zAxis;
		else referencePos = pos - halfExtents.z*zAxis;
		u = halfExtents.x*xAxis;
		v = halfExtents.y*yAxis;
	}

	if (counterClockwise) {
		vertices[0] = referencePos + u + v;
		vertices[1] = referencePos - u + v;
		vertices[2] = referencePos - u - v;
		vertices[3] = referencePos + u - v;
	} else {
		vertices[0] = referencePos + u + v;
		vertices[1] = referencePos + u - v;
		vertices[2] = referencePos - u - v;
		vertices[3] = referencePos - u + v;
	}
}

vector<Vector3> PhysicsManager::ClipFaceToSidePlane(vector<Vector3> &clipPolygon, const Vector3& normal, float plane_offset)
{
	Vector3 Vertex1 = clipPolygon.back();
	float Distance1 = Vertex1.dotAKAscalar(normal) - plane_offset;//rnDistance(Plane, Vertex1.Position);
	vector<Vector3> newClipPolygon;
	for (size_t i = 0; i < clipPolygon.size(); i++)
	{
		Vector3 Vertex2 = clipPolygon[i];
		float Distance2 = Vertex2.dotAKAscalar(normal) - plane_offset;//rnDistance(Plane, Vertex2.Position);

		if (Distance1 <= 0.0f && Distance2 <= 0.0f)
		{
			// Both vertices are behind the plane - keep vertex2
			newClipPolygon.push_back(Vertex2);
		}
		else if (Distance1 <= 0.0f && Distance2 > 0.0f)
		{
			// Vertex1 is behind the plane, vertex2 is in front -> intersection point
			float Fraction = Distance1 / (Distance1 - Distance2);
			Vector3 newVertex = Vertex1 + Fraction * (Vertex2 - Vertex1);

			// Keep intersection point 
			newClipPolygon.push_back(newVertex);
		}
		else if (Distance2 <= 0.0f && Distance1 > 0)
		{
			// Vertex2 is behind the plane, vertex1 is in front -> intersection point
			float Fraction = Distance1 / (Distance1 - Distance2);
			Vector3 newVertex1 = Vertex1 + Fraction * (Vertex2 - Vertex1);

			// Keep intersection point 
			newClipPolygon.push_back(newVertex1);

			// And also keep vertex2
			newClipPolygon.push_back(Vertex2);
		}

		// Keep vertex2 as starting vertex for next edge
		Vertex1 = Vertex2;
		Distance1 = Distance2;
	}
	return newClipPolygon;
}

inline void PhysicsManager::CreateSidePlanesOffsetsAndNormals(const Vector3& onePosition, int typeOfCollision, Vector3 &normal1, const Matrix3 &one, Vector3 &normal2, float &neg_offset1, Vector3 oneHalfSize, float &pos_offset1, float &neg_offset2, float &pos_offset2)
{
	//get normal for all face and it's offset
	switch (typeOfCollision)
	{
	case 0: //skip x
	{
		//we get y and z axis
		normal1 = one.getAxis(1);
		normal2 = one.getAxis(2);
		float y = onePosition.dotAKAscalar(normal1);  //returns distance from origo in normal1 direction
		float z = onePosition.dotAKAscalar(normal2);
		neg_offset1 = -y + oneHalfSize[1];
		//float myNegOff = (onePosition - normal1*oneHalfSize[1]).dotAKAscalar(-1.f*normal1); //now that's the correct distance of the plane from origo we must use correct oriented normal when dotting
		pos_offset1 = y + oneHalfSize[1];
		neg_offset2 = -z + oneHalfSize[2];
		pos_offset2 = z + oneHalfSize[2];
		/*
		//so that hax method above works below is more logical way
		neg_offset1 = (onePosition - normal1*oneHalfSize[1]).dotAKAscalar(-1.f*normal1); 
		pos_offset1 = (onePosition + normal1*oneHalfSize[1]).dotAKAscalar(normal1);
		neg_offset2 = (onePosition - normal2*oneHalfSize[2]).dotAKAscalar(-1.f*normal2);
		pos_offset2 = (onePosition + normal2*oneHalfSize[2]).dotAKAscalar(normal2);
		*/

		DrawSidePlanes(normal1, normal2, onePosition, 1, 2, oneHalfSize);
	}
	break;

	case 1: //skip y
	{
		//we get x and z axis
		normal1 = one.getAxis(0);
		normal2 = one.getAxis(2);
		float x = onePosition.dotAKAscalar(normal1);
		float z = onePosition.dotAKAscalar(normal2);
		neg_offset1 = -x + oneHalfSize[0];
		pos_offset1 = x + oneHalfSize[0];
		neg_offset2 = -z + oneHalfSize[2];
		pos_offset2 = z + oneHalfSize[2];

		DrawSidePlanes(normal1, normal2, onePosition, 0, 2, oneHalfSize);
	}
	break;

	case 2: //skip z
	{
		//we get x and y axis
		normal1 = one.getAxis(0);
		normal2 = one.getAxis(1);
		float x = onePosition.dotAKAscalar(normal1);
		float y = onePosition.dotAKAscalar(normal2);
		neg_offset1 = -x + oneHalfSize[0];
		pos_offset1 = x + oneHalfSize[0];
		neg_offset2 = -y + oneHalfSize[1];
		pos_offset2 = y + oneHalfSize[1];

		DrawSidePlanes(normal1, normal2, onePosition, 0, 1, oneHalfSize);
	}
	break;
	}
}

inline void PhysicsManager::ClaculateIncidentAxis(Vector3& incident_axis, const Matrix3 &two, Vector3 smallestAxis)
{
	float incident_tracker = FLT_MAX;
	//get incident face vertices by dotting all axis with mtv
	for (int i = 0; i < 3; i++)
	{
		float most_neg = two.getAxis(i).dotAKAscalar(smallestAxis);
		if (most_neg < incident_tracker){
			incident_tracker = most_neg;
			incident_axis = two.getAxis(i);
		}
		most_neg = (-1.f * two.getAxis(i)).dotAKAscalar(smallestAxis);
		if (most_neg < incident_tracker){
			incident_tracker = most_neg;
			incident_axis = -1.f * two.getAxis(i);
		}
	}
}

inline void PhysicsManager::FilterContactsAgainstReferenceFace(vector<Vector3> &contacts, const Vector3& refNormal, float pos_offsett, unordered_set<Contact*> &contact_points_out, float penetration, Object* one, Object* two)
{
	//this step is supposed to test against the reference plane all the contact points and we keep only the points inside or at the cube
	//this is to clean up contacts 
	for (size_t i = 0; i < contacts.size(); i++)
	{
		//float Distance1 = Vector3::Dot(contacts[i], -1 * refNormal) - neg_offsett;
		//if (Distance1 <= 0){
		//	//keep point
		//	contact_points_out.push_back(contacts[i]);
		//}
		float Distance2 = contacts[i].dotAKAscalar(refNormal) - pos_offsett;
		if (Distance2 <= 0){
			//keep point
			Contact* contact = new Contact();
			contact->contactPoint = contacts[i];
			contact->penetration = -1.f*Distance2; //penetration per contact
			//contact->penetration = penetration; //mtv penetration
			contact->contactNormal = refNormal;
			contact->one = one;
			contact->two = two;
			contact_points_out.insert(contact);
		}
	}
}

inline void PhysicsManager::FlipMTVTest(Vector3 &mtv, const Vector3 & toCentre)
{
	// We know which axis the collision is on (i.e. best),
	// but we need to work out which of the two faces on
	// this axis. //flipping
	float toCentreDist = mtv.dotAKAscalar(toCentre);
	if (toCentreDist < 0) //have turned sign!
	{
		mtv = mtv * -1.0f;
	}
}

void PhysicsManager::DrawSidePlanes(const Vector3& normal1, const Vector3& normal2, const Vector3& onePosition, int index1, int index2, const Vector3& oneHalfSize)
{
	if (DebugDraw::Instance()->debug)
	{
		Vector3 vertexOnPlane1 = onePosition - normal1*oneHalfSize.vect[index1]; // -1*normal
		Vector3 vertexOnPlane2 = onePosition + normal1*oneHalfSize.vect[index1];
		Vector3 vertexOnPlane3 = onePosition - normal2*oneHalfSize.vect[index2]; // -1*normal
		Vector3 vertexOnPlane4 = onePosition + normal2*oneHalfSize.vect[index2];
		//let's draw calculated planes with normals at plane points
		DebugDraw::Instance()->line.mat->SetColor(1, 0, 0); //red
		DebugDraw::Instance()->point.mat->SetColor(1, 0, 1);
		DebugDraw::Instance()->DrawPlaneN(-1.f*normal1, vertexOnPlane1);

		DebugDraw::Instance()->line.mat->SetColor(0, 1, 0); //green
		DebugDraw::Instance()->point.mat->SetColor(1, 0, 1);
		DebugDraw::Instance()->DrawPlaneN(normal1, vertexOnPlane2);

		DebugDraw::Instance()->line.mat->SetColor(0, 0, 1); //blue
		DebugDraw::Instance()->point.mat->SetColor(1, 0, 1);
		DebugDraw::Instance()->DrawPlaneN(-1.f*normal2, vertexOnPlane3);

		DebugDraw::Instance()->line.mat->SetColor(1, 1, 0); //yellow
		DebugDraw::Instance()->point.mat->SetColor(1, 0, 1);
		DebugDraw::Instance()->DrawPlaneN(normal2, vertexOnPlane4);
	}
}

void PhysicsManager::DrawReferenceAndIncidentFace(Vector3 * reference_face, Vector3 * incident_face)
{
	//reference face verts
	DebugDraw::Instance()->point.mat->SetColor(0, 1, 1);
	DebugDraw::Instance()->DrawPoint(reference_face[0], 15);
	DebugDraw::Instance()->DrawPoint(reference_face[1], 15);
	DebugDraw::Instance()->DrawPoint(reference_face[2], 15);
	DebugDraw::Instance()->DrawPoint(reference_face[3], 15);
	
	//draw incident face verts
	DebugDraw::Instance()->point.mat->SetColor(1, 0, 0);
	DebugDraw::Instance()->DrawPoint(incident_face[0], 15); //upper left
	DebugDraw::Instance()->DrawPoint(incident_face[1], 15); //lower left
	DebugDraw::Instance()->DrawPoint(incident_face[2], 15); //lower right
	DebugDraw::Instance()->DrawPoint(incident_face[3], 15); //upper right
}

void PhysicsManager::DrawCollisionNormal(Contact* contact)
{
	//collision normal
	DebugDraw::Instance()->line.mat->SetColor(0, 0, 0); //black 
	DebugDraw::Instance()->point.mat->SetColor(0, 0, 0);
	DebugDraw::Instance()->DrawNormal(contact->contactNormal, contact->two->node.position); //draw mtv after flip
}

void PhysicsManager::DrawReferenceNormal(Contact* contact, int typeOfCollision)
{
	//reference normal
	Vector3 centerPointOfRefFace = contact->one->node.position + contact->contactNormal*contact->one->obb.halfExtent[typeOfCollision];
	DebugDraw::Instance()->point.mat->SetColor(1, 1, 1); //white
	DebugDraw::Instance()->line.mat->SetColor(1, 1, 1);
	DebugDraw::Instance()->DrawNormal(contact->contactNormal, centerPointOfRefFace); //draw reference normal
}

void PhysicsManager::DrawFaceDebug(unordered_set<Contact*> &contacts, Vector3 * reference_face, Vector3 * incident_face, int typeOfCollision)
{
	if (contacts.size() > 0)
	{
		Contact* contact = *contacts.begin();
		DrawCollisionNormal(contact);
		DrawReferenceNormal(contact, typeOfCollision);
		DrawReferenceAndIncidentFace(reference_face, incident_face);
	}
}

size_t PhysicsManager::DrawPlaneClipContacts(vector<Vector3> &contacts, const Vector3& normal, size_t vertCount, const Vector3& normalColor)
{
	for (size_t i = vertCount; i < contacts.size(); i++)
	{
		DebugDraw::Instance()->point.mat->SetColor(1, 0, 1);
		DebugDraw::Instance()->line.mat->SetColor(normalColor);
		DebugDraw::Instance()->DrawNormal(normal, contacts[i]);
	}	
	vertCount = contacts.size(); 
	return vertCount;
}

void PhysicsManager::Clear()
{
	for (size_t i = 0; i < xAxis.size(); i++)
	{
		delete xAxis[i];
		delete yAxis[i];
		delete zAxis[i];
	}
	xAxis.clear();
	yAxis.clear();
	zAxis.clear();
	satOverlaps.clear();
	fullOverlaps.clear();
}


