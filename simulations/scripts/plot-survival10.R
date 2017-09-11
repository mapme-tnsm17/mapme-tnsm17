#!/usr/bin/env Rscript

library(ggplot2)
library(RSvgDevice)

source("scripts/theme_custom.R")

colors <- list("0.7"="green", "0.8"="red", "1.2"="blue")


for (alpha in c("0.7", "0.8")) {#, "1.2")) {
    temp_dfm <- read.table(paste("./data/r0.5-K10000-C10-s100-a", alpha, "-d100000/1/survival.out", sep=''))
    temp_dfm$alpha <- as.factor(alpha)
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

# Mean survival time per object rank
# Input format: ITEM OBJ SEQ POPULARITY SURVIVAL
ddfm <- aggregate(dfm$V5, list(rank=dfm$V2, alpha=dfm$alpha), mean)
summary(ddfm)

g <- ggplot(ddfm, aes(x=rank, y=x, group=alpha, colour=alpha)) + 
        geom_line() +
        scale_x_log10() +
        scale_y_log10() +
        xlab("Content rank") +
        ylab("Mean survival time (s)") +
        theme_custom
devSVG("plots/survival10.svg")
print(g)
dev.off()
