#include "lattice.h"
using namespace std;

#define tau 1

double d2q9_weights[] = {1.0/36, 1.0/9, 1.0/36, 1.0/9, 4.0/9, 1.0/9, 1.0/36, 1.0/9, 1.0/36};

Lattice::Lattice(const int x_size, const int y_size):
    XDIM(x_size), YDIM(y_size), NUM_SITES(XDIM*YDIM), NUM_WEIGHTS(9), 
    weight(d2q9_weights, d2q9_weights + NUM_WEIGHTS) {
  f_density.resize(
      boost::extents[range(-2, NUM_SITES)][range(1, NUM_WEIGHTS+1)]);
  push_density.resize(
      boost::extents[range(-2, NUM_SITES)][range(1, NUM_WEIGHTS+1)]);
  neighbors.resize(
      boost::extents[range(-2, NUM_SITES)][range(1, NUM_WEIGHTS+1)]);
  node_state.resize(
      boost::extents[range(-2, NUM_SITES)]);


  buildNeighbors();
  setStates();

  return;
}

double Lattice::density(const int site) {
  double rho = 0;
  for(int n = 1; n <= NUM_WEIGHTS; n++) rho += f_density[site][n];
  return rho;
}

void Lattice::update() {
  streamingUpdate();
  collisionUpdate();
  return;
}

void Lattice::print(ostream &os) {
  for(int y = YDIM - 1; y >= 0; y--) {
    for(int x = 0; x < XDIM; x++) {
      int site = coord2idx(Eigen::Vector2i(x, y));
      os << density(site);
      if(x != XDIM - 1) os << ",";
    }
    os << endl;
  }
  return;
}

Eigen::Vector2i Lattice::idx2coord(const int idx) {
  int x = idx % XDIM, y = idx/XDIM;
  return Eigen::Vector2i(x, y);
}

int Lattice::coord2idx(Eigen::Vector2i r) {
  return r[0] + r[1]*XDIM;
}

void Lattice::setStates() {
  for(int site = 0; site < NUM_SITES; site++) {
    node_state[site] = ACTIVE;
    for(int n = 1; n <= NUM_WEIGHTS; n++)
      f_density[site][n] = weight[n - 1];
  }

  f_density[XDIM + 1][3] = 1.1*f_density[XDIM+1][3];

}

void Lattice::buildNeighbors() {
  for(int site = 0; site < NUM_SITES; site++) {
    Eigen::Vector2i r(idx2coord(site));

    for(int n = 1; n <= NUM_WEIGHTS; n++) {

      Eigen::Vector2i r = idx2coord(site), dr = directionToSteps(n),
                      rprime = r + dr;

      rprime[0] = (rprime[0] <  0    ? rprime[0] + XDIM : rprime[0]);
      rprime[0] = (rprime[0] >= XDIM ? rprime[0] - XDIM : rprime[0]);

      rprime[1] = (rprime[1] <  0    ? rprime[1] + XDIM : rprime[1]);
      rprime[1] = (rprime[1] >= XDIM ? rprime[1] - XDIM : rprime[1]);

      neighbors[site][n] = coord2idx(rprime);
    }
  }

  return;
}

void Lattice::streamingUpdate() {
  for(int site = 0; site < NUM_SITES; site++) {
    if(node_state[site] == INACTIVE) continue;

    for(int n = 1; n <= NUM_WEIGHTS; n++) {
      int neighbor_idx = neighbors[site][n];

      if(node_state[neighbor_idx] == INACTIVE) {
        push_density[site][(NUM_WEIGHTS + 1) - n] = f_density[site][n];
      } else {
        push_density[neighbor_idx][n] = f_density[site][n];
      }
    }
  }
  f_density = push_density;
  return;
}

void Lattice::collisionUpdate() {
  for(int site = 0; site < NUM_SITES; site++) {
    Eigen::Vector2d macro_vel(0,0);
    double density = 0;

    for(int n = 1; n <= NUM_WEIGHTS; n++) {
      macro_vel += f_density[site][n]*(directionToSteps(n).cast<double>());
      density += f_density[site][n];
    }

    if(density != 0) macro_vel /= density;

    for(int n = 1; n <= NUM_WEIGHTS; n++) {
      double e_dot_u = directionToSteps(n).cast<double>().dot(macro_vel);
      double equilibrium = (1 + 3.0*e_dot_u + (9.0/2)*pow(e_dot_u,2)
        - (3.0/2)*macro_vel.squaredNorm())*weight[n - 1]*density; // c = 1
               //weight is a std::vector here^, hence n - 1
        
      f_density[site][n] -= 1.0/tau*(f_density[site][n] - equilibrium);
    }
  }

  return;
}

Eigen::Vector2i directionToSteps(const int n) {
  if (n < 1 || n > 9 ) {
    throw domain_error("Direction out of bounds");
  }
  int count = 0;
  Eigen::Vector2i result;
  for(int dy = -1; dy <= 1; dy++) {
    for(int dx = -1; dx <= 1; dx++) {
      if (++count == n)
        result = Eigen::Vector2i(dx, dy);
    }
  }
  return result;
}
