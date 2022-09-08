/*************************************************************************
 * $Id: Graph.hxx,v 1.5 2000/11/14 06:15:03 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:15:03 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 11/14/99 by Charles Chiou
 * Comments: Graph interface class.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _GRAPH_HXX_
#define _GRAPH_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"
#include "String.hxx"
#include "Slist.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI GraphException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(GraphException);

public:

    GraphException();
    GraphException(const char *msg);

};

class UTILAPI Vertex : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(Vertex);

public:

    Vertex(const char *id = NULL);

    const char *operator()() const;

    boolean operator==(const Vertex &v) const;

protected:

    String _id;

};

class UTILAPI Edge : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(Edge);

public:

    Edge();
    Edge(const Vertex &v1, const Vertex &v2, const char *label,
         boolean directed = FALSE);

    boolean isDirected() const;

    Vertex source() const;       // If undirected, then the first vertex
    Vertex destination() const;  // If undirected, then the second vertex

    const char *label() const;
    const char *operator()() const;
    void setLable(const char *label);

    boolean operator==(const Edge &edge) const;

protected:

    boolean isVisited() const;
    void visit();
    void reset();

    Vertex _v1;
    Vertex _v2;
    String _label;
    boolean _directed;
    boolean _visited;

};

//////////////////////////////////////////////////////////////////////////
// Interfaces classes                                                   //
//////////////////////////////////////////////////////////////////////////

/**
 * Undirected graph interface functions.
 */
class UTILAPI Graph : public Root {

public:

    virtual int numVertices() const = 0;
    virtual int numEdges() const = 0;

    virtual Slist<Vertex> vertices() const = 0;
    virtual Slist<Edge> edges() const = 0;

    virtual int degree(const Vertex &v) const throw(GraphException) = 0;

    virtual Slist<Vertex> adjacentVertices(const Vertex &v) const
        throw(GraphException) = 0;
    virtual Slist<Edge> incidentEdges(const Vertex &v) const
        throw(GraphException) = 0;

    virtual boolean areAdjacent(const Vertex &v1, const Vertex &v2) const
        throw(GraphException) = 0;

    virtual void insertVertex(const Vertex &v) throw(GraphException) = 0;
    virtual void insertEdge(const Edge &e) throw(GraphException) = 0;

    virtual void removeVertex(const Vertex &v) throw(GraphException) = 0;
    virtual void removeEdge(const Edge &e) throw(GraphException) = 0;

};

/**
 * Directed graph interface functions.
 */
class UTILAPI DiGraph : public Root {

public:

    virtual Slist<Edge> directedEdges() const = 0;
    virtual Slist<Edge> undirectedEdges() const = 0;

    virtual int inDegree(const Vertex &v) const throw(GraphException) = 0;
    virtual int outDegree(const Vertex &v) const throw(GraphException) = 0;

    virtual Slist<Vertex> inAdjacentVertices(const Vertex &v) const
        throw(GraphException) = 0;
    virtual Slist<Vertex> outAdjacentVertices(const Vertex &v) const
        throw(GraphException) = 0;
    virtual Slist<Edge> inIncidentEdges(const Vertex &v) const
        throw(GraphException) = 0;
    virtual Slist<Edge> outIncidentEdges(const Vertex &v) const
        throw(GraphException) = 0;

    virtual void markUndirected(const Edge &edge) throw(GraphException) = 0;
    virtual void reverseDirection(const Edge &edge) throw(GraphException) = 0;
    virtual void setDirectionFrom(const Edge &edge, const Vertex &v)
        throw(GraphException) = 0;
    virtual void setDirectionTo(const Edge &edge, const Vertex &v)
        throw(GraphException) = 0;

};

//////////////////////////////////////////////////////////////////////////
// Implementation using adjacency matrix                                //
//////////////////////////////////////////////////////////////////////////

/**
 * Undirected graph with adjacency matrix data structure.
 */
class UTILAPI MatrixGraph : public BaseObject, public Graph {

    DECLARE_RUNTIME_SERIALIZABLE(MatrixGraph);

public:

    int numVertices() const;
    int numEdges() const;

    Slist<Vertex> vertices() const;
    Slist<Edge> edges() const;

    int degree(const Vertex &v) const throw(GraphException);

    Slist<Vertex> adjacentVertices(const Vertex &v) const throw(GraphException);
    Slist<Edge> incidentEdges(const Vertex &v) const throw(GraphException);

    boolean areAdjacent(const Vertex &v1, const Vertex &v2) const
        throw(GraphException);

    void insertVertex(const Vertex &v) throw(GraphException);
    void insertEdge(const Edge &e) throw(GraphException);

    void removeVertex(const Vertex &v) throw(GraphException);
    void removeEdge(const Edge &e) throw(GraphException);

protected:

};

/**
 * Directed graph with adjacency matrix data structure.
 */
class UTILAPI MatirxDiGraph : public MatrixGraph, public DiGraph {

    DECLARE_RUNTIME_SERIALIZABLE(MatirxDiGraph);

public:

    Slist<Edge> directedEdges() const;
    Slist<Edge> undirectedEdges() const;

    int inDegree(const Vertex &v) const throw(GraphException);
    int outDegree(const Vertex &v) const throw(GraphException);

    Slist<Vertex> inAdjacentVertices(const Vertex &v) const
        throw(GraphException);
    Slist<Vertex> outAdjacentVertices(const Vertex &v) const
        throw(GraphException);
    Slist<Edge> inIncidentEdges(const Vertex &v) const throw(GraphException);
    Slist<Edge> outIncidentEdges(const Vertex &v) const throw(GraphException);

    void markUndirected(const Edge &edge) throw(GraphException);
    void reverseDirection(const Edge &edge) throw(GraphException);
    void setDirectionFrom(const Edge &edge, const Vertex &v)
        throw(GraphException);
    void setDirectionTo(const Edge &edge, const Vertex &v)
        throw(GraphException);

};

//////////////////////////////////////////////////////////////////////////
// Implementation using adjacency list                                  //
//////////////////////////////////////////////////////////////////////////

/**
 * Undirected graph with adjacency list data structure.
 */
class UTILAPI ListGraph : public BaseObject, public Graph {

    DECLARE_RUNTIME_SERIALIZABLE(ListGraph);

public:

    int numVertices() const;
    int numEdges() const;

    Slist<Vertex> vertices() const;
    Slist<Edge> edges() const;

    int degree(const Vertex &v) const throw(GraphException);

    Slist<Vertex> adjacentVertices(const Vertex &v) const throw(GraphException);
    Slist<Edge> incidentEdges(const Vertex &v) const throw(GraphException);

    boolean areAdjacent(const Vertex &v1, const Vertex &v2) const
        throw(GraphException);

    void insertVertex(const Vertex &v) throw(GraphException);
    void insertEdge(const Edge &e) throw(GraphException);

    void removeVertex(const Vertex &v) throw(GraphException);
    void removeEdge(const Edge &e) throw(GraphException);

protected:

};

/**
 * Undirected graph with adjacency list data structure.
 */
class UTILAPI ListDiGraph : public ListGraph {

    DECLARE_RUNTIME_SERIALIZABLE(ListDiGraph);

public:

    Slist<Edge> directedEdges() const;
    Slist<Edge> undirectedEdges() const;

    int inDegree(const Vertex &v) const throw(GraphException);
    int outDegree(const Vertex &v) const throw(GraphException);

    Slist<Vertex> inAdjacentVertices(const Vertex &v) const
        throw(GraphException);
    Slist<Vertex> outAdjacentVertices(const Vertex &v) const
        throw(GraphException);
    Slist<Edge> inIncidentEdges(const Vertex &v) const throw(GraphException);
    Slist<Edge> outIncidentEdges(const Vertex &v) const throw(GraphException);

    void markUndirected(const Edge &edge) throw(GraphException);
    void reverseDirection(const Edge &edge) throw(GraphException);
    void setDirectionFrom(const Edge &edge, const Vertex &v)
        throw(GraphException);
    void setDirectionTo(const Edge &edge, const Vertex &v)
        throw(GraphException);

};

// Inline functions.

inline GraphException::GraphException() : Exception() {

}

inline GraphException::GraphException(const char *msg) : Exception(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
