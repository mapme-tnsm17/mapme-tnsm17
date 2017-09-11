#!/usr/bin/env Rscript

library(ggplot2)
library(RSvgDevice)

source("scripts/theme_custom.R")

fn0 <- "data/r0.5-K10000-C10-s100-a0.7-d100000/1/load"
fn1 <- "data/r0.5-K10000-C10-s100-a0.8-d100000/1/load"
fn2 <- "data/r0.5-K10000-C10-s100-a1.2-d100000/1/load"

dfm <- read.table(fn0)
dfm$alpha <- factor(0.7)
temp_dfm <- read.table(fn1)
temp_dfm$alpha <- factor(0.8)
dfm <- rbind(dfm, temp_dfm)
temp_dfm <- read.table(fn2)
temp_dfm$alpha <- factor(1.2)
dfm <- rbind(dfm, temp_dfm)

dfm$fraction <- dfm$V3 * 100 / dfm$V2

g_base <- ggplot(dfm, aes(V1, fraction, group=alpha, color = alpha)) + 
        geom_line() + 
        xlab("Time (s)") +
        ylab("Fraction of load served by caches (%)") +
        xlim(0, 50000) +
        theme_custom

g <- g_base + ylim(0, 100)
devSVG("plots/loadcache10.svg")
print(g)
dev.off()

g <- g_base + ylim(0, 10)
devSVG("plots/loadcache10-zoom.svg")
print(g)
dev.off()


