Starcraft 2 Build Order Simulator
====

This code is the result of the lecture 'Advanced Programming Techniques' due
Winter Term 2016/2017.

Contributors:
----
Christian Bay (christian.bay@fau.de)
Cuong Bui     (cuong.bui@fau.de)
Malte Kraus   (malte.kraus@fau.de)


What is it about
---

The goal of this project was it to simulate and optimize one aspect of the early game, the pro-
duction of buildings and units. The creation of buildings consumes time and resources, and possibly
requires a certain technology, which is represented by the presence of other building types. Creation
of units also occupies a building as production site for limited time, and generally requires supply,
which is provided by some buildings.

The simulation is done by using a constant time step (unit time), because the duration of all actions
are be specified in full seconds. It is assumed that one can initiate at most one game action per second.

Resources
----

Three types of resources are common to all races: Mineral, Vespene gas, as well as supply.

The start configuration always includes a few workers (SCV/Probe/Drone) and a basic building close
to mineral patches and a Vespene geyser, but a special building (Refinery/Extractor/Assimilator) is
necessary to harvest Vespene gas.

The following simple approach is used to simulate resource farming accyrately:

- Minerals are excavated at an average rate per harvesting worker (∼ 0.7 /s).
- Vespene gas is similarly harvested according to average rates (∼ 0.35 /s), but not more than three workers must be assigned to this job simultaneously per tapped geyser. There are only two geysers available per base.


Races
----

Zergs, Terran and Protoss have distinct abilities, which are not listed in detail here.

Usage
---

There are different modes that can be simulated

1. Forward simulation:

	At first, the program checks if the build list is valid and in case it is, it tries to figure
	out the fastest way to build the entities given in the `buildlistFile` in the correct order.

	`./forwardSim <race> <buildlistFile>`
	
2. Rush strategy:

	A player wants as many units of a certain type as possible at a certain point in game time to attack and obliterate or at least decimate his forces.

	`./optimize.sh rush zergling 360`

3. Push stragegy:

	Focusses on developing advanced technology as fast as possible, merely holding the enemy at bay for that time. If successful, superior units can defeat an opponent, even if outnumbered by less developed units.

	`./optimize.sh push battlecruiser 2`


For the *push* and *rush* strategy a genetic algorithm has been implemented.

Output
----

The program prints the build order as JSON file. The output consists of *messages* in the following
form:

```json
{ "time" : <current simulation time in seconds (integer)>,
"status" : { "resources" :
{ "minerals" : <minerals, rounded down to next integer>,
"vespene" : <vespene, rounded down to next integer>,
"supply" : <total supply available>,
"supply-used": <supply used by all your units> },
"workers" :
{ "minerals" : <number of workers collecting minerals>,
"vespene" : <number of workers collecting vespene> }
}
"events" : <list of events, for event specification see below>,
}
```

Events occur either when the creation of a unit/building is started or finished, or when a special ability
is triggered.

**Build-start event:**

```json
{ "type" : "build-start",
"name" : <name of unit or building>,
"producerID": <id of producer>
}
```

Example for a special ability event:

**Terran:**

```json
{ "type" : "special",
"name" : "mule",
"triggeredBy" : <id of building triggering the mule>
}
```
