/*************************************************************************
 * $Id: Graph.cxx,v 1.6 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 11/14/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Graph.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(GraphException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Vertex, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Edge, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(MatrixGraph, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(MatrixDiGraph, MatrixGraph, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(ListGraph, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(ListDiGraph, ListGraph, 1);

Vertex::Vertex(const char *id)
{
    _id = id;
}

Edge::Edge()
{

}

Edge::Edge(const Vertex &v1, const Vertex &v2, const char *label,
           boolean directed)
{
    _v1 = v1;
    _v2 = v2;
    _label = label;
    _directed = directed;
    _visited = 0;
}

int MatrixGraph::numVertices() const
{

}

int MatrixGraph::numEdges() const
{
    throw Exception("Unimplemented");
}

Slist<Vertex> MatrixGraph::vertices() const
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixGraph::edges() const
{
    throw Exception("Unimplemented");
}

int MatrixGraph::degree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> MatrixGraph::adjacentVertices(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixGraph::incidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

boolean MatrixGraph::areAdjacent(const Vertex &v1, const Vertex &v2) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixGraph::insertVertex(const Vertex &v) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixGraph::insertEdge(const Edge &e) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixGraph::removeVertex(const Vertex &v) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixGraph::removeEdge(const Edge &e) throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixDiGraph::directedEdges() const
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixDiGraph::undirectedEdges() const
{
    throw Exception("Unimplemented");
}

int MatrixDiGraph::inDegree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

int MatrixDiGraph::outDegree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> MatrixDiGraph::inAdjacentVertices(const Vertex &v) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> MatrixDiGraph::outAdjacentVertices(const Vertex &v) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixDiGraph::inIncidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> MatrixDiGraph::outIncidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixDiGraph::markUndirected(const Edge &edge) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixDiGraph::reverseDirection(const Edge &edge) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixDiGraph::setDirectionFrom(const Edge &edge, const Vertex &v)
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

void MatrixDiGraph::setDirectionTo(const Edge &edge, const Vertex &v)
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

int ListGraph::numVertices() const
{
    throw Exception("Unimplemented");
}

int ListGraph::numEdges() const
{
    throw Exception("Unimplemented");
}

Slist<Vertex> ListGraph::vertices() const
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListGraph::edges() const
{
    throw Exception("Unimplemented");
}

int ListGraph::degree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> ListGraph::adjacentVertices(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListGraph::incidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

boolean ListGraph::areAdjacent(const Vertex &v1, const Vertex &v2) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListGraph::insertVertex(const Vertex &v) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListGraph::insertEdge(const Edge &e) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListGraph::removeVertex(const Vertex &v) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListGraph::removeEdge(const Edge &e) throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListDiGraph::directedEdges() const
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListDiGraph::undirectedEdges() const
{
    throw Exception("Unimplemented");
}

int ListDiGraph::inDegree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

int ListDiGraph::outDegree(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> ListDiGraph::inAdjacentVertices(const Vertex &v) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Vertex> ListDiGraph::outAdjacentVertices(const Vertex &v) const
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListDiGraph::inIncidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

Slist<Edge> ListDiGraph::outIncidentEdges(const Vertex &v) const throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListDiGraph::markUndirected(const Edge &edge) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListDiGraph::reverseDirection(const Edge &edge) throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListDiGraph::setDirectionFrom(const Edge &edge, const Vertex &v)
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

void ListDiGraph::setDirectionTo(const Edge &edge, const Vertex &v)
    throw(GraphException)
{
    throw Exception("Unimplemented");
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
