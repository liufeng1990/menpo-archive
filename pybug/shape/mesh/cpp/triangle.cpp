#include <iostream>
#include <iomanip>
#include <set>
#include "halfedge.h"
#include "triangle.h"
#include "vertex.h"

Triangle::Triangle(Mesh* mesh, unsigned tri_id, Vertex* v0,
        Vertex* v1, Vertex* v2) : MeshAttribute(mesh, tri_id) {
    v0_ = v0;
    v1_ = v1;
    v2_ = v2;
    // we need to know if any of the edges are going to be flipped when
    // connected up - i.e. do any of these halfedges already exist?
    bool e0_flip = v0_->halfedge_to_vertex(v1_) ? 1 : 0;
    bool e1_flip = v1_->halfedge_to_vertex(v2_) ? 1 : 0;
    bool e2_flip = v2_->halfedge_to_vertex(v0_) ? 1 : 0;
    // if, in addition to having a half edge going this direction already
    // existing (which we can fix with a flip) we have another halfedge
    // made, then flipping won't help - there is no way to ensure chirality.
    // we mark these edges, and deal with them after mesh construction is complete.
    if (e0_flip && v1_->halfedge_to_vertex(v0_)) {
        std::cout << this << " is over determined on e0 (" << v0_ << "-"
                << v1_ << ")" << std::endl;
        e0_flip = false; // no point in flipping now
    }
    if (e1_flip && v2_->halfedge_to_vertex(v1_)) {
        std::cout << this << " is over determined on e1 (" << v1_ << "-"
                << v2_ << ")" << std::endl;
        e1_flip = false;
    }
    if (e2_flip && v0_->halfedge_to_vertex(v2_)) {
        std::cout << this << " is over determined on e2 (" << v2_ << "-"
                << v0_ << ")" << std::endl;
        e2_flip = false;
    }
    // create all the new halfedges - we need these for this
    // triangle to make sense
    e0_ = createHalfedge(v0_, v1_, v2_, 0);
    e1_ = createHalfedge(v1_, v2_, v0_, 1);
    e2_ = createHalfedge(v2_, v0_, v1_, 2);
    // deal with any that need flipping
    if (e0_flip || e0_flip || e0_flip)
        resolveChirality(e0_flip, e1_flip, e2_flip);
    // now, we should definitely have a well formed triangle.
    // lets check...
    if (!(v0_->legal_attachment_to_tri(*this) &&
          v1_->legal_attachment_to_tri(*this) &&
          v2_->legal_attachment_to_tri(*this)))
        std::cout << this << " has a vertex error." << std::endl;
}

Triangle::~Triangle() {}


Vertex* Triangle::get_v0() {
    return v0_;
}

Vertex* Triangle::get_v1() {
    return v1_;
}

Vertex* Triangle::get_v2() {
    return v2_;
}

void Triangle::set_v0(Vertex* value) {
    v0_ = value;
}

void Triangle::set_v1(Vertex* value) {
    v1_ = value;
}

void Triangle::set_v2(Vertex* value) {
    v2_ = value;
}

Halfedge* Triangle::get_e0() {
    return e0_;
}

Halfedge* Triangle::get_e1() {
    return e1_;
}

Halfedge* Triangle::get_e2() {
    return e2_;
}

void Triangle::set_e0(Halfedge* value) {
    e0_ = value;
}

void Triangle::set_e1(Halfedge* value) {
    e1_ = value;
}

void Triangle::set_e2(Halfedge* value) {
    e2_ = value;
}


Halfedge* Triangle::createHalfedge(Vertex* v0, Vertex* v1, Vertex* v2,
                                   unsigned halfedge_id) {
    v0->add_tri(this);
    v0->add_vertex(v1);
    v1->add_vertex(v0);
    Halfedge* halfedge = new Halfedge(this->get_mesh(), v0, v1, v2, this, halfedge_id);
    v0->add_halfedge(halfedge);
    //if (!v0->halfedge_to_vertex(v1))
    //    std::cout << "just added a halfedge that I cannot find" << std::endl;
   return halfedge;
}

void Triangle::resolveChirality(bool e0_bad, bool e1_bad, bool e2_bad) {
    std::cout << this << " - resolving chirality error against ";
    if (e0_bad)
        std::cout << get_e0()->paired_tri() << " with " << get_e0() << "/" << get_e0()->paired_he();
    if (e1_bad)
        std::cout << get_e1()->paired_tri() << " with " << get_e1() << "/" << get_e1()->paired_he();
    if (e2_bad)
        std::cout << get_e2()->paired_tri() << " with " << get_e2() << "/" << get_e2()->paired_he();
    std::cout << std::endl;
    // temporarily store the connecting pointers to other regions
    Halfedge *h0, *h1, *h2;
    // store out the current e0,e1,e2 (their meaning will change with the flip
    // - we just want to ensure that the correct edges get wired up at the
    // end!)
    Halfedge *orig_e0, *orig_e1, *orig_e2;
    orig_e0 = e0_;
    orig_e1 = e1_;
    orig_e2 = e2_;
    // detach any bad halfedges so we don't recursively flip onto a 'good' set
    // of triangles
    if (e0_bad) {
        h0 = e0_->paired_he();
        e0_->set_paired_he(NULL);
    } if (e1_bad) {
        h1 = e1_->paired_he();
        e1_->set_paired_he(NULL);
    } if (e2_bad) {
        h2 = e2_->paired_he();
        e2_->set_paired_he(NULL);
    }
    // call flip on myself, flipping myself and all my neighbours.
    this->flipContiguousRegion();
    // now myself and all my (non bad) neighbours are flipped, I reattach the
    // bad edges. Everything is now fixed
    if (e0_bad)
        orig_e0->set_paired_he(h0);
    if (e1_bad)
        orig_e1->set_paired_he(h1);
    if (e2_bad)
        orig_e2->set_paired_he(h2);
}

Triangle* Triangle::t0() {
    return e0_->paired_tri();
}

Triangle* Triangle::t1() {
    return e1_->paired_tri();
}

Triangle* Triangle::t2() {
    return e2_->paired_tri();
}

std::set<Triangle *> Triangle::adjacent_triangles() {
    std::set<Triangle *> triangles;
    triangles.insert(t0());
    triangles.insert(t1());
    triangles.insert(t2());
    triangles.erase(NULL);
    return triangles;
}

void Triangle::flipContiguousRegion() {
    std::set<Triangle*>* visited_tris = new std::set<Triangle*>;
    visited_tris->insert(this);
    // call recursive flip on ourselves
    this->recursiveFlip(visited_tris);
    // all done - clean up the memory
    delete visited_tris;
}

void Triangle::recursiveFlip(std::set<Triangle*>* visited_tris) {
    visited_tris->insert(this);  // add myself to the list
    //std::cout << this << " - recursive flip" << std::endl;
    // flip the meaning of each half edge around.
    e0_->flip();
    e1_->flip();
    e2_->flip();
    // now the state of halfedges and vertices are fixed up, but this triangle
    // is all wrong.
    // make sure v0, v1, v2 are correct in meaning by flipping v0, v1_
    Vertex *v0_temp = v0_;
    v0_ = v1_;
    v1_ = v0_temp;
    // make sure e0, e1, e2 have the correct meaning by flipping e1_, e2_
    Halfedge *e1_temp = e1_;
    e1_ = e2_;
    e2_ = e1_temp;
    // update the get_id()'s on the halfedges themselves to be correct
    e1_->set_id(e1_->get_id() + 1); // e1_ -> e2_
    e2_->set_id(e1_->get_id() - 1); // e2 -> e1
    // get each neighbouring triangle, other than the one who called us
    std::set<Triangle*> adjacent_tris = adjacent_triangles();
    std::set<Triangle*>::iterator it;
    for (it = adjacent_tris.begin(); it != adjacent_tris.end(); it++) {
        if (!visited_tris->count(*it))
            (*it)->recursiveFlip(visited_tris);  // haven't been here before
    }
}

void Triangle::reduce_scalar_to_vertices(double* triangle_scalar,
        double* vertex_scalar) {
    vertex_scalar[v0_->get_id()] += triangle_scalar[get_id()];
    vertex_scalar[v1_->get_id()] += triangle_scalar[get_id() + 1];
    vertex_scalar[v2_->get_id()] += triangle_scalar[get_id() + 2];
}

void Triangle::reduce_scalar_per_vertex_to_vertices(
        double* triangle_scalar_per_vertex, double* vertex_scalar) {
    vertex_scalar[v0_->get_id()] += triangle_scalar_per_vertex[(get_id() * 3)];
    vertex_scalar[v1_->get_id()] += triangle_scalar_per_vertex[(get_id() * 3) + 1];
    vertex_scalar[v2_->get_id()] += triangle_scalar_per_vertex[(get_id() * 3) + 2];
}

void Triangle::status() {
    std::cout << "    TRIANGLE " << get_id() << "        " << std::endl;
    Halfedge* h01 = v0_->halfedge_on_tri(this);
    Halfedge* h12 = v1_->halfedge_on_tri(this);
    Halfedge* h20 = v2_->halfedge_on_tri(this);
    unsigned width = 12;
    std::cout  << std::setw(width) << "V0(" << v0_->get_id() << ")";
    if (h01->part_of_fulledge()) {
        std::cout << "============";
    }
    else {
        std::cout << "------------";
    }
    std::cout  << std::setw(width) << "V1(" << v1_->get_id() << ")";
    if (h12->part_of_fulledge()) {
        std::cout << "============";
    }
    else {
        std::cout << "------------";
    }
    std::cout  << std::setw(width) << "V2(" << v2_->get_id() << ")";
    if (h20->part_of_fulledge()) {
        std::cout << "============";
    }
    else {
        std::cout << "------------";
    }
    std::cout << std::setw(width) << "V0(" << v0_->get_id() << ")" << std::endl;

    std::cout  << std::setw(width) << " ";
    if (h01->part_of_fulledge()) {
        std::cout  << std::setw(width) << h01->paired_tri()->get_id();
    }
    else {
        std::cout << " -- ";
    }
    std::cout  << std::setw(width) << " ";
    if (h12->part_of_fulledge()) {
        std::cout << std::setw(width) << h12->paired_tri()->get_id();
    }
    else {
        std::cout << " -- ";
    }
    std::cout  << std::setw(width) << " ";
    if (h20->part_of_fulledge()) {
        std::cout << std::setw(width) << h20->paired_tri()->get_id();
    }
    else {
        std::cout << " -- ";
    }
}

