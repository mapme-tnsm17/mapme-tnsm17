#!/usr/bin/env Rscript
library(ggplot2)
library(RSvgDevice)

input=commandArgs(TRUE)[1]
output=commandArgs(TRUE)[2]

t <- read.table(paste(input, 'popularity-c1', sep=''))
devSVG(paste(output, 'zipf-c1.svg', sep=''))
ggplot(t, aes(seq_along(V1), V1)) + geom_line() + scale_x_log10() + scale_y_log10() + annotation_logticks()
dev.off()

# Why is this not working for alpha
model.lm <- lm(V1 ~ seq_along(V1), data = t)
model.lm

