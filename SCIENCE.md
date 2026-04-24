# Math and Physics Notes

## Model

The simulation evolves a complex wavefunction $\psi(x, y, t)$ on a 2D grid.

It uses the time-dependent Schrodinger equation in dimensionless form:

$$
i\frac{\partial \psi}{\partial t} = -\frac{1}{2}\nabla^2\psi + V(x,y)\psi
$$

Where:

- $\psi$ is the quantum state
- $|\psi|^2$ is probability density
- $V(x, y)$ is the potential field (painted barriers, walls, slits)

## Numerical method

The update is an operator split (Strang splitting):

$$
e^{-i(T+V)\Delta t} \approx e^{-iV\Delta t/2}\,e^{-iT\Delta t}\,e^{-iV\Delta t/2}
$$

with kinetic operator $T=-\frac{1}{2}\nabla^2$.

For each substep, the solver does:

1. Half-step potential phase: multiply by $e^{-iV\Delta t/2}$
2. Kinetic step from the Laplacian term
3. Half-step potential phase again

The potential phase is pointwise:

$$
\psi \leftarrow \psi\,e^{-iV\Delta t/2}
$$

The kinetic step uses a Crank-Nicolson form. For

$$
\frac{\partial \psi}{\partial t}=\frac{i}{2}\nabla^2\psi
$$

CN gives

$$
\left(I-\frac{i\Delta t}{4}\nabla^2\right)\psi^{n+1}
=
\left(I+\frac{i\Delta t}{4}\nabla^2\right)\psi^n
$$

On the grid, with spacings $\Delta x, \Delta y$,

$$
\nabla^2\psi_{i,j}
=
\frac{\psi_{i+1,j}-2\psi_{i,j}+\psi_{i-1,j}}{\Delta x^2}
+
\frac{\psi_{i,j+1}-2\psi_{i,j}+\psi_{i,j-1}}{\Delta y^2}
$$

The resulting linear system is solved iteratively (Gauss-Seidel style) with a fixed iteration count (`Solver iters` in the UI).

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
