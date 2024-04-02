#pragma once

#include <Eigen/Dense>
//
#include <cinolib/gradient.h>
#include <cinolib/heat_flow.h>
#include <cinolib/meshes/meshes.h>
#include <cinolib/scalar_field.h>
#include <cinolib/vector_field.h>

namespace ensketch::sandbox {

// This is a demo implementation of the hyper surface relaxation described in
//
// Marco Livesu:
// A Heat Flow Based Relaxation Scheme for n Dimensional Discrete Hyper Surfaces,
// Computers and Graphics, 2018
//
using namespace cinolib;
//
template <class Mesh>
ScalarField smooth_discrete_hyper_surface(Mesh& m) {
  constexpr auto LAMBDA = 0.1;
  constexpr auto SMOOTHING_PASSES = 5;

  // STEP ONE: compute heat flow
  std::vector<uint> heat_sources;
  for (uint vid = 0; vid < m.num_verts(); ++vid) {
    bool has_A = false;
    bool has_B = false;
    for (uint pid : m.adj_v2p(vid)) {
      if (m.poly_data(pid).label == 0)
        has_A = true;
      else if (m.poly_data(pid).label == 1)
        has_B = true;
    }
    if (has_A && has_B) heat_sources.push_back(vid);
  }
  double t = std::pow(m.edge_avg_length(), 2.0);
  ScalarField u = heat_flow(m, heat_sources, t, COTANGENT);
  u.normalize_in_01();
  u.copy_to_mesh(m);
  u.serialize("u.txt");
  std::cout << "heat flow computed (see file u.txt)" << std::endl;

  VectorField field = VectorField(m.num_polys());
  field = gradient_matrix(m) * u;
  field.serialize("u_gradient.txt");
  std::cout << "u gradient computed (see file u_gradient.txt)" << std::endl;

  // STEP TWO: flip the gradient of one of the regions
  for (uint pid = 0; pid < m.num_polys(); ++pid) {
    if (m.poly_data(pid).label == 1) field.set(pid, -field.vec_at(pid));
  }
  field.normalize();
  field.serialize("X.txt");
  std::cout << "X field generated (see file X.txt)" << std::endl;

  // STEP THREE: smooth the resulting gradient
  for (uint i = 0; i < SMOOTHING_PASSES; ++i)
    for (uint pid = 0; pid < m.num_polys(); ++pid) {
      vec3d avg_g = field.vec_at(pid);
      for (uint nbr : m.adj_p2p(pid)) {
        avg_g += field.vec_at(nbr);
      }
      avg_g /= static_cast<double>(m.adj_p2p(pid).size() + 1);
      avg_g.normalize();
      field.set(pid, avg_g);
    }
  field.normalize();
  field.serialize("X_prime.txt");
  std::cout << "smoothed X field generated (see file X_prime.txt)" << std::endl;

  // STEP FOUR: find the scalar field corresponding to it
  int type = UNIFORM;
  if (m.mesh_type() == TRIMESH || m.mesh_type() == TETMESH)
    type = COTANGENT;  // use cotangent weights for tris and tets
  std::vector<Eigen::Triplet<double>> entries =
      laplacian_matrix_entries(m, type);
  Eigen::VectorXd div = gradient_matrix(m).transpose() * field;
  Eigen::SparseMatrix<double> L(m.num_verts() + heat_sources.size(),
                                m.num_verts());
  Eigen::VectorXd rhs(m.num_verts() + heat_sources.size());
  for (uint vid = 0; vid < m.num_verts(); ++vid) rhs[vid] = div[vid];
  for (uint i = 0; i < heat_sources.size(); ++i) {
    uint vid = heat_sources.at(i);
    entries.push_back(Entry(m.num_verts() + i, vid, LAMBDA));
    rhs[m.num_verts() + i] = 0.0;
  }
  L.setFromTriplets(entries.begin(), entries.end());
  ScalarField phi;
  solve_least_squares(-L, rhs, phi);
  phi.copy_to_mesh(m);
  phi.normalize_in_01();
  return phi;
}

}  // namespace ensketch::sandbox
