#
# Makefile to reproduce plots in the paper:
# MAP-Me: Managing Anchor-less Producer Mobility in Information Centric Networks
# Sec. V. Evaluation
#
SHELL:=/bin/bash

all: plots

simulations: simulations-speed simulations-baseline simulations-tu

simulations-speed:
	./do speed

simulations-baseline:
	./do baseline

simulations-tu:
	./do tu

plots: simulations
	./scripts/new-plot.R speed
	./scripts/new-plot.R baseline
	./scripts/plot-rates.R baseline
	./scripts/plot-tu.R tu
