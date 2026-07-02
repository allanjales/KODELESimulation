# KODEL-E Garfield

## Setup

All stack is placed with z = 0 at center

### Layer Stack

It is needed to create a vector using the struct `Layer` which associates a Medium ([see also](https://garfieldpp.docs.cern.ch/doxygen/classGarfield_1_1Medium.html)) with a thickness.

A example of this usage can be seen at `KODELE.cc`

### Setup method

After creating a instance for the class, you must call they `Setup()` method to construct all the scenario.

### Visualization

The `signalTimeWindow` carries the visualization information of the signal.
You can define its parameters by using `signalTimeWindow.Set(tmin, tmax, nbins)`.

Other useful visualization you can extract before simulation are `PlotElectricFieldProfile()` which will plot the profile (in Z direction) of the electric field only in the active medium, which is basically where there are gas.

The `PlotWeightingFieldProfile()` will plot the Weighting field.

The `PlotDriftLines2D()` will plot the electrons drift paths in the AvalancheMicroscopic part, which consists of a few moments after simulation start only, so it will not describe its full trajectory.

`PlotSignal()` will plot signal and `PlotCharge()` will plot the total charge collected by the sensor.

### Simulating

Before the simulation, you will need to add one or multiple tracks to begin with using the `AddTrack()` method passing as argument the particle name, its momentum, its start position, its direction and its starting time.

Finally, with everything set up, run

```cpp
Simulate()
```

to get all informations you need.

After the run, the track list is cleared, so you will need to `AddTrack()` again.

By default all results are stored in `results/` folder, but you can use `SetOutputFolder()` function to set a new path destination

## Simulation principle

First part of simulation it runs using AvalancheMicroscopic, which will simulate the trajectory of each individual electrons for 1 nanosecond. Then, due to the high increasing number of electrons, its switchs for the AvalancheGrid method, a faster way to simulate same results from avalanche using previous calculated coefficients for gases in a `.gas` file.