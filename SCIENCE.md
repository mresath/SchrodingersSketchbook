# Math and Physics Notes

## Model

The simulation evolves a complex wavefunction $\psi(x, y, t)$ on a 2D grid.

It uses the time-dependent Schrödinger equation:

$$
i\hbar\frac{\partial \psi}{\partial t} = -\frac{\hbar^2}{2m}\nabla^2\psi + V(x,y)\psi
$$

Where:

- $\psi$ is the quantum state
- $|\psi|^2$ is probability density
- $V(x, y)$ is the potential field (painted barriers, walls, slits)
- $T=-\frac{\hbar}{2m}\nabla^2$ is the kinetic operator, with laplacian $\nabla^2$.
- $H=T+V$ is the Hamiltonian which corresponds to the total energy of the system.
- $m$ is the particle mass
- $\hbar$ is the reduced Planck constant
- $i$ is the imaginary unit.

The equation is discretized on a domain with characteristic units, and $\hbar$ and $m$ are tunable parameters (by default set to 1.0 in dimensionless simulations but adjustable to change the physics: higher $\hbar$ increases dispersion, while higher $m$ reduces the kinetic energy scale).

## Numerical method

The update is an operator split (Strang splitting):

$$
e^{-i(T+V)\Delta t/\hbar} \approx e^{-iV\Delta t/(2\hbar)}\,e^{-iT\Delta t/\hbar}\,e^{-iV\Delta t/(2\hbar)}
$$

For each substep, the solver does:

1. Half-step potential phase: multiply by $e^{-iV\Delta t/(2\hbar)}$
2. Kinetic step from the Laplacian term
3. Half-step potential phase again

The potential phase is pointwise:

$$
\psi \leftarrow \psi\,e^{-iV\Delta t/(2\hbar)}
$$

The kinetic step uses a Crank-Nicolson form. For

$$
\frac{\partial \psi}{\partial t}=\frac{i\hbar}{2m}\nabla^2\psi
$$

CN gives

$$
\left(I-\frac{i\hbar\Delta t}{4m}\nabla^2\right)\psi^{n+1}
=
\left(I+\frac{i\hbar\Delta t}{4m}\nabla^2\right)\psi^n
$$

On the grid, with spacings $\Delta x, \Delta y$,

$$
\nabla^2\psi_{i,j}
=
\frac{\psi_{i+1,j}-2\psi_{i,j}+\psi_{i-1,j}}{\Delta x^2}
+
\frac{\psi_{i,j+1}-2\psi_{i,j}+\psi_{i,j-1}}{\Delta y^2}
$$

The solver discretizes this system and solves iteratively (Gauss-Seidel style) with a fixed iteration count (`Solver iters` in the UI).

Important details:

- If `Substeps` = $s$, the solver uses $\Delta t_{sub} = \Delta t / s$.
- Higher `Solver iters` gives a more accurate implicit solve but costs more CPU.
- Lower `Solver iters` is faster but adds more numerical error.

## Boundaries

Grid edges are set to $\psi=0$ each step (hard boundary). This reflects wave content at edges.

## Renormalization

The norm is:

$$
N = \sum |\psi|^2\,\Delta x\,\Delta y
$$

When renormalization is enabled, the state is scaled after stepping:

$$
\psi \leftarrow \frac{\psi}{\sqrt{N}}
$$

This keeps total probability near 1.

## What this means in the app

- Higher momentum gives faster phase variation and directed travel.
- Higher barrier potential increases reflection and reduces transmission.
- Double-slit patterns come from interference after passing through two openings.
