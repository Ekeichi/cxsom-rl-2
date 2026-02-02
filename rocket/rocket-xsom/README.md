Here, we set up 3 connected SOMs. Each one is connected to the two others by a forward and a feedback connection. Let us experiment this. We have customized a [makefile](makefile) for that purpose, read it.

## Known Issues & troubleshooting

During the development of this experiment, we encountered and resolved two critical issues:

1.  **Empty Prediction Output**:
    *   **Problem**: `make show-predictions` fails with `RuntimeError: No mappable was found` or shows "Loaded 1 points".
    *   **Cause**: Running `make predict WEIGHTS_AT=30000` when the training hasn't reached step 30000. The processor waits indefinitely for weight files that don't exist.
    *   **Solution**: Ensure you have fully trained the model (`make feed-train-inputs WALLTIME=30000` and wait for completion) before predicting at that timestep. Alternatively, check available weights in `root-dir/saved` and use a valid `WEIGHTS_AT` value.

## Description

This experiments builds up 3 1D maps `som_error`, `som_velocity` and `som_thrust`. They are fed with data from a rocket controller.
The scalar `error` feeds `som_error`, the scalar `velocity` feeds `som_velocity` and the scalar `thrust` feeds `som_thrust`.

The three maps are reciprocally connected. The idea is to learn the relation between `error`, `velocity` and `thrust`, and then ask the map to deduce `thrust` from `error` and `velocity`.

The demo is organized into several stages, detailed below.

Make sure to have the venv activated before running the makefile.

## Set up the demo

First setup a root-dir directory for our variables as well as the virtual env.

```
mkdir root-dir
make cxsom-set-config ROOT_DIR=./root-dir VENV=../cxsom-venv HOSTNAME=localhost PORT=10000 SKEDNET_PORT=20000 NB_THREADS=4
```

Last we can launch a processor, and scan the root-dir.

```
make cxsom-launch-processor
make cxsom-scan-vars
```

The demo (with its different stages) is define in the [xsom.cpp](xsom.cpp) file.

## The calibration stage.

This relies on two python scripts, [set-calibration.py](set-calibration.py) and [show-calibration.py](show-calibration.py). This consists in exhibiting the tuning curves, as defined by the parameters set in the [xsom.cpp](xsom.cpp) code. This invokes the cxsom-processor, but this is not a real simulation.

```
make clear-calibration
make cxsom-clear-processor
make calibration-setup GRID_SIDE=100
make calibrate
make show-calibration 
```

## The input stage

First, we have to set up the dataset that corresponds to the function `(error, velocity) -> thrust` that we want to learn. This uses [build-rocket-dataset.py](build-rocket-dataset.py) and [show-samples.py](show-samples.py).

```
make inputs-setup
make show-samples
```

This computes the input samples, and feeds the corresponding variables.

Training consists of choosing a random value in this pre-computed set, and feed the maps with it.

## The training stage

Now we can train. Let us first see how training computation is done.

```
make show-train-rules
```

Let us emplace the training rules at server side (the null walltime warning is ok). Check the variable scanning and wait for the termination of the computing.

```
make train-setup SAVE_PERIOD=1000 DATA_SIZE=2601
```

Indeed, computation is only done at step 0. This is due to the walltime value of the rule setting train-in/coord. In order to trigger computation until timestep 30000, we just have to send a rule that modifies thes walltime.

```
make feed-train-inputs WALLTIME=30000
```

Once again, check the variable scanning and wait for the termination of the computing.

You can check if the training seems ok after this (thanks to [show-weights-history.py](show-weights-history.py)). If not, feed again with a higher walltime.

```
make show-weights-history
```

Once you are ok, you can clear the training stuff. Once you do this, the training cannot be continued for further steps. If you do not type 'make clear-training', you can continue the training further on (see Restarting subsection next).

```
make cxsom-clear-processor
make clear-training
```

### Restarting

If you need to restart and continue the computation (up to 100000 for example)

```
make cxsom-launch-processor
make train-setup SAVE_PERIOD=1000 DATA_SIZE=2601
make feed-train-inputs WALLTIME=100000
```

## The ckecking stage

In this mode, we check wether the map is able to encode in its external weights a good representation of the data. To do so, we feed the map with data triplets, and we get the corresponding bmus. The rules for this are the following:

```
make show-check-rules
```

We can, for example, build up the checking.

```
make cxsom-clear-processor
make clear-checks
make check WEIGHTS_AT=30000 DATA_SIZE=2601
```

Check the variable scanning and wait for the termination of the computing. Then, let us display the prediction

```
make show-checks
```

Clearing the checkings can be done as well.

```
make cxsom-clear-processor
make clear-checks
```

## Predict mode

Here, we ask the map to retrieve thrust from (error, velocity) values. Let us first display the prediction rules.

```
make show-predict-rules
```

An then let us build-up a prediction for the saved weights at 30000.

```
make cxsom-clear-processor
make clear-predictions
make predict WEIGHTS_AT=30000 DATA_SIZE=2601
```

Check the variable scanning and wait for the termination of the computing. Then, let us display the prediction (see [show-rocket-predictions.py](show-rocket-predictions.py)).

```
make show-predictions
```

Clearing the prediction can be done as well.

```
make cxsom-clear-processor
make clear-predictions
```

## Making movies (at work)

See the instructions for this:

```
make movie-help
```

This relies on ffmpeg, as well as [frame-factory.py](frame-factory.py), [make-frame.py](make-frame.py), [wait_stable.py](wait_stable.py), [weights-frames.py](weights-frames.py).
