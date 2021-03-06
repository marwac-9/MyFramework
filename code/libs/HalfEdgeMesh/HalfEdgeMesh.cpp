#include <vector>
#include <map>
#include "HalfEdgeMesh.h"
#include "OBJ.h"
#include "Vertex.h"
#include "Edge.h"
#include "Face.h"

using namespace cop4530;


HalfEdgeMesh::HalfEdgeMesh()
{
	vertexPool.CreatePoolParty();
	edgePool.CreatePoolParty();
	facePool.CreatePoolParty();
}

HalfEdgeMesh::~HalfEdgeMesh()
{
}

void HalfEdgeMesh::Construct(OBJ &object)
{

	//copy all data from vectors to half edge mesh vectors
	//vertice data
	for (size_t i = 0; i < object.indexed_vertices.size(); i++)
	{
		Vertex* newVertex = vertexPool.PoolPartyAlloc();
		newVertex->pos = object.indexed_vertices.at(i);
		newVertex->normal = object.indexed_normals.at(i);
		newVertex->tex = object.indexed_uvs.at(i);
		vertices.push_back(newVertex);
	}

	//connecting
	for (size_t i = 0; i < object.indices.size(); i += 3)
	{
		//get vertex index from element array containing indices
		int vertexIndex = object.indices.at(i);
		int vertexIndex2 = object.indices.at(i + 1);
		int vertexIndex3 = object.indices.at(i + 2);
		//let's get vertices we wanna work with
		Vertex* vertice1 = vertices.at(vertexIndex);
		Vertex* vertice2 = vertices.at(vertexIndex2);
		Vertex* vertice3 = vertices.at(vertexIndex3);


		//create new edges
		Edge* newEdge1 = edgePool.PoolPartyAlloc();
		Edge* newEdge2 = edgePool.PoolPartyAlloc();
		Edge* newEdge3 = edgePool.PoolPartyAlloc();

		//connect vertices to edges
		newEdge1->vertex = vertice1;
		newEdge2->vertex = vertice2;
		newEdge3->vertex = vertice3;

		//connect edges to vertices
		vertice1->edge = newEdge1;
		vertice2->edge = newEdge2;
		vertice3->edge = newEdge3;

		//connect edges with next
		newEdge1->next = newEdge2;
		newEdge2->next = newEdge3;
		newEdge3->next = newEdge1;

		edges.push_back(newEdge1);
		edges.push_back(newEdge2);
		edges.push_back(newEdge3);

	}

	//connect edges to faces and faces to edges
	for (int i = 0; i < edges.size(); i+=3)
	{
		Face* newFace = facePool.PoolPartyAlloc();
		newFace->edge = edges.at(i);

		//edges.at(i).face = &faces.at(i);
		//edges.at(i+1).face = &faces.at(i);
		//edges.at(i+2).face = &faces.at(i);

		edges.at(i)->face = newFace;
		edges.at(i)->next->face = newFace;
		edges.at(i)->next->next->face = newFace;

		faces.push_back(newFace);
	}

	//find pairs
	for (auto& edgei : edges)
	{	
		for (auto& edgej : edges)
		{
			if (edgej->vertex->pos == edgei->next->vertex->pos) //we could just compare pointers but we have double vertices, not per triangle but per quad
			{
				if (edgej->next->vertex->pos == edgei->vertex->pos)
				{
					edgei->pair = edgej;
					edgej->pair = edgei;
					break;
				}
			}
		}
	}

	/*
	for (int i = 0; i < edges.size(); i++)
	{
		if (edges.at(i)->next == edges.at(i)->pair)
		{
			printf("error %d \n", i);
		}
	}
	*/
}

bool HalfEdgeMesh::checkIfSameVect(Vector3 &vect1, Vector3 &vect2)
{
	if (vect1.vect[0] == vect2.vect[0] && vect1.vect[1] == vect2.vect[1] && vect1.vect[2] == vect2.vect[2]) {
		return true;
	}
	else{
		return false;
	}
}

void HalfEdgeMesh::Subdivide()
{
	CalculateOldPosition();
	CalculateMidpointPosition();
	SplitHalfEdges();
	UpdateConnections();
	UpdateVertexPositions();
	//we could do the new recalculation of normals 
	//for each vertex we find all neighbours
	//then we calculate new normals from them
	//traversing code is already existent in CalculateOldPosition function
}


void HalfEdgeMesh::CalculateOldPosition()
{
	for (int i = 0; i < edges.size(); i++)
	{
		if (edges[i]->next == edges[i]->pair)
		{
			printf("\nFEL\n");
		}
	}

	for (int i = 0; i < vertices.size(); i++){
		//Get the vertex to which we will calculate its new position
		Vertex* vertex = vertices.at(i);
		//Get the half-edge of the vertex we are working on
		Edge* halfedge = vertices.at(i)->edge;

		//Sum of all neighbour vertices
		Vector3F neighbourSum;
		//Amount of neighbors around the vertex
		float n = 0;

		//we find here all neighboring vertices of current vertex of current edge, we can use this traversing for better calculation of normals after subdivision 
		//starting edge, its vertex
		Edge* currentEdge = halfedge->next;
		//traverse around vertice sum neighboring vertices
		do
		{
			neighbourSum = neighbourSum + currentEdge->vertex->pos;
			n++;

			//get next neighbour
			currentEdge = currentEdge->next->pair->next;
		} while (currentEdge != halfedge->next);

		//Calculate b which is a function used by the equation which calculates the new position
		//Using  Warren and Weimer's equation which doesnt have expensive trigonometric functions
		float b = 3.0f / (n*(n + 2.0f));

		vertex->newPos = (1.0f - n*b)*vertex->pos + b*neighbourSum;
	}
}

void HalfEdgeMesh::CalculateMidpointPosition()
{
	/*
	Iterate edges calculate the midpoint vertex
	*/

	/*Iterate HE around face . do while until same HE then pair.next.next.origin. after do while jaha. then got all 4 vertices*/
	unsigned int halfEdgeListSize = edges.size();
	for (unsigned int i = 0; i < halfEdgeListSize; i++){
		//Get the halfedge to which we will calculate midpoint position of the new vertex
		Edge* halfedge = edges.at(i);

		if (halfedge->midVertex == nullptr){
			//to calculate midpoint position i need first all involved vertices
			Vertex* v1 = halfedge->vertex;
			Vertex* v2 = halfedge->next->vertex;
			Vertex* v3 = halfedge->next->next->vertex;
			Vertex* v4 = halfedge->pair->next->next->vertex;

			Vector3F vertexPosSum = 3.f * (v1->pos + v2->pos) + v3->pos + v4->pos;

			//we only calculate midpoint and assign it to the edge and it's pair
			//we are not performing any splitting or reconnections here
			Vertex* midpoint = vertexPool.PoolPartyAlloc();
			Vector3F calculatedPosition = vertexPosSum / 8.0f;
			Vector2F calculatedUVs = (v1->tex + v2->tex) / 2.0f;
			Vector3F calculatedNormal = (v1->normal + v2->normal + v3->normal + v4->normal) / 4.0f;
			midpoint->pos = calculatedPosition; // there is no reason to set this as we copy newPos to pos later on
			midpoint->tex = calculatedUVs;
			midpoint->normal = calculatedNormal.vectNormalize();
			midpoint->newPos = calculatedPosition;
			halfedge->midVertex = midpoint;
			halfedge->pair->midVertex = midpoint;


			vertices.push_back(midpoint);
		}
	}
}

void HalfEdgeMesh::SplitHalfEdges()
{
	//here we want to split edges and connect pairs
	unsigned int hEdgesSize = edges.size();
	//iterate trought edge list and split edge if not splitted 
	for (unsigned int i = 0; i < hEdgesSize; i++){
		//Create new half-edge
		Edge* newHEdge = edgePool.PoolPartyAlloc();
		newHEdge->vertex = edges.at(i)->midVertex;
		newHEdge->vertex->edge = newHEdge;
		newHEdge->next = edges.at(i)->next;
		newHEdge->face = edges.at(i)->face;

		edges.at(i)->next = newHEdge;
		newHEdge->pair = edges.at(i)->pair;

		edges.push_back(newHEdge);
	}

	for (int i = hEdgesSize; i < edges.size(); i++){
		edges.at(i)->pair->pair = edges.at(i);
	}
}

void HalfEdgeMesh::UpdateVertexPositions()
{
	//we copy new positions into original positions
	for (int i = 0; i < vertices.size(); i++){
		vertices.at(i)->pos = vertices.at(i)->newPos;
	}
}

void HalfEdgeMesh::UpdateConnections()
{
	unsigned int faceListSize = faces.size();
	for (unsigned int i = 0; i < faceListSize; i++){
		Face* leftFace = faces.at(i);
		Face* topFace = facePool.PoolPartyAlloc();
		Face* rightFace = facePool.PoolPartyAlloc();
		Face* middleFace = facePool.PoolPartyAlloc();

		Edge* innerLeftEdge = edgePool.PoolPartyAlloc();
		Edge* innerTopEdge = edgePool.PoolPartyAlloc();
		Edge* innerRightEdge = edgePool.PoolPartyAlloc();

		//New triangle, middle
		Edge* innerRightMidEdge = edgePool.PoolPartyAlloc();
		Edge* innerLeftMidEdge = edgePool.PoolPartyAlloc();
		Edge* innerBottomMidEdge = edgePool.PoolPartyAlloc();

		Edge* oldLeftEdge = faces.at(i)->edge;
		Edge* newLeftEdge = faces.at(i)->edge->next;
		Edge* oldRightEdge = faces.at(i)->edge->next->next;
		Edge* newRightEdge = faces.at(i)->edge->next->next->next;
		Edge* oldBottomEdge = faces.at(i)->edge->next->next->next->next;
		Edge* newBottomEdge = faces.at(i)->edge->next->next->next->next->next;



		//Connectivity for inner edges
		//Bottom left face
		innerLeftEdge->vertex = newLeftEdge->vertex;  

		//connectivity
		innerLeftEdge->next = newBottomEdge;
		oldLeftEdge->next = innerLeftEdge;

		innerLeftEdge->face = leftFace; 

		//fixing pair for the inner middle face
		innerLeftEdge->pair = innerLeftMidEdge;
		innerLeftMidEdge->pair = innerLeftEdge; 

		//Top face
		topFace->edge = newLeftEdge; 
		innerTopEdge->vertex = newRightEdge->vertex; 

		//connectivity
		innerTopEdge->next = newLeftEdge; 
		oldRightEdge->next = innerTopEdge; 

		//setting all edges to the new top face
		newLeftEdge->face = topFace;
		oldRightEdge->face = topFace;
		innerTopEdge->face = topFace; 

		//fixing pair for the inner middle face
		innerTopEdge->pair = innerBottomMidEdge;
		innerBottomMidEdge->pair = innerTopEdge; 

		//Bottom right face
		rightFace->edge = newRightEdge;
		innerRightEdge->vertex = newBottomEdge->vertex; 

		innerRightEdge->next = newRightEdge; 
		oldBottomEdge->next = innerRightEdge; 

		//setting all edges to the new right face
		newRightEdge->face = rightFace;
		oldBottomEdge->face = rightFace;
		innerRightEdge->face = rightFace; 

		//fixing pair for the inner middle face
		innerRightEdge->pair = innerRightMidEdge;
		innerRightMidEdge->pair = innerRightEdge; 

		//Middle face
		middleFace->edge = innerRightMidEdge; 
		innerRightMidEdge->vertex = newRightEdge->vertex; 
		innerRightMidEdge->next = innerLeftMidEdge; 

		innerLeftMidEdge->vertex = newBottomEdge->vertex;
		innerLeftMidEdge->next = innerBottomMidEdge; 

		innerBottomMidEdge->vertex = newLeftEdge->vertex;  
		innerBottomMidEdge->next = innerRightMidEdge; 

		//setting all edges to the new middle face
		innerRightMidEdge->face = middleFace;
		innerLeftMidEdge->face = middleFace;
		innerBottomMidEdge->face = middleFace;

		//pushing new faces
		faces.push_back(topFace);
		faces.push_back(rightFace);
		faces.push_back(middleFace);

		//pushing new edges
		edges.push_back(innerLeftEdge);
		edges.push_back(innerTopEdge);
		edges.push_back(innerRightEdge);
		edges.push_back(innerRightMidEdge);
		edges.push_back(innerLeftMidEdge);
		edges.push_back(innerBottomMidEdge);
	}
}


void HalfEdgeMesh::ExportMeshToOBJ(HalfEdgeMesh* mesh, OBJ* newOBJ)
{

	//GENERATING RENDERABLE DATA FORMAT 

	//let's use map for vertex pointer and index 
	std::map<const Vertex*, int> indexMap;

	//for each vertice export position normal and uv
	for (int i = 0; i < mesh->vertices.size(); i++)
	{
		newOBJ->indexed_vertices.push_back(mesh->vertices.at(i)->pos);
		newOBJ->indexed_uvs.push_back(mesh->vertices.at(i)->tex);
		newOBJ->indexed_normals.push_back(mesh->vertices.at(i)->normal);
		indexMap[mesh->vertices.at(i)] = i;
	}

	for (int i = 0; i < mesh->faces.size(); i++){
		Edge* faceTraverser = mesh->faces.at(i)->edge;
		do
		{
			int index;
			//Iterator
			//we search for the same vertex in vertex index map
			std::map<const Vertex*, int>::iterator it = indexMap.find(faceTraverser->vertex);
			index = it->second;
			//then we push back it's index
			newOBJ->indices.push_back(index);

			faceTraverser = faceTraverser->next;
		} while (faceTraverser != mesh->faces.at(i)->edge);
	}
}