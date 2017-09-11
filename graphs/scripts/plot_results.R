#!/usr/bin/env Rscript

library(ggplot2)
source('scripts/theme_custom.R')

AS <- c("1221", "1239", "1755", "2914", "3257", "3356", "3967", "4755", "6461", "7018")

# bleu rouge jaune violet vert
colors <- c("#00b0f5ff", "#f8756bff", "#a1a300ff", "#e668f3ff", "#00bd7dff")
colors <- c("#f8756bff", "#a1a300ff", "#e668f3ff", "#00bd7dff")

files <- dir("results", pattern = "rocket.*_.*_.*", full.names = TRUE)
print(files)
tables <- lapply(files, function(f) { read.table(f, header=TRUE) })
dfm <- do.call(rbind, tables)

# Rename topology levels
for (i in AS) {
    levels(dfm$topology)[levels(dfm$topology)==paste("rocketfuel-", i, "-r0.cch", sep="")] <- i
}

# Additional columns
dfm$max <- dfm$mean + dfm$ci
dfm$min <- dfm$mean - dfm$ci

dfm$solution <- as.character(dfm$solution)
dfm$solution[dfm$solution == "anchor"] <- "AB"
dfm$solution[dfm$solution == "map-me"] <- "MapMe-IU"
dfm$solution[dfm$solution == "kite"] <- "TB"
dfm$solution <- factor(dfm$solution, levels=c("AB", "TB", "MapMe-IU"))
dfm$movement <- as.numeric(dfm$movement)

#sdfm <- subset(dfm, metric=="stretch" & solution=="anchor" & movement==2)
#ggplot(sdfm, aes(movement, mean, group=interaction(topology, pattern), color=pattern)) + geom_line() + xlim(0,100)

# Comparison of approaches for 1 movement with uniform mobility
##sdfm <- subset(dfm, metric=="stretch" & pattern=="uniform" & movement==500)
##svg("plots/stretch-uniform.svg")
##ggplot(sdfm, aes(topology, mean, fill=solution)) +
##    geom_bar(stat="identity", position="dodge", width=0.5) + 
##    ylim(0, 3) +
###    geom_errorbar(aes(ymax=max, ymin=min), position=position_dodge(0.9)) +
##    theme_custom
##dev.off()

# Comparison of approaches for 1 movement with waypoint mobility
sdfm <- subset(dfm, metric=="stretch" & pattern=="waypoint" & solution!="ours" & movement==500)
svg("plots/stretch-waypoint.svg")
ggplot(sdfm, aes(topology, mean, fill=solution)) +
    geom_bar(stat="identity", position="dodge", width=0.5) + 
    ylim(0, 3) +
    xlab("Rocketfuel AS number") +
    ylab("Average path stretch") +
    scale_fill_manual(values = colors) +
#    geom_errorbar(aes(ymax=max, ymin=min), position=position_dodge(0.9)) +
    theme_custom +
    theme(legend.title=element_blank(),  axis.text.x = element_text(size=16))
dev.off()

# Comparison of approaches for 1 movement with waypoint mobility
sdfm <- subset(dfm, metric=="stretch" & pattern=="uniform" & solution!="ours" & movement==500)
svg("plots/stretch-uniform.svg")
ggplot(sdfm, aes(topology, mean, fill=solution)) +
    geom_bar(stat="identity", position="dodge", width=0.5) + 
    ylim(0, 3) +
    xlab("Rocketfuel AS number") +
    ylab("Average path stretch") +
    scale_fill_manual(values = colors) +
#    geom_errorbar(aes(ymax=max, ymin=min), position=position_dodge(0.9)) +
    theme_custom +
    theme(legend.title=element_blank(),  axis.text.x = element_text(size=16))
dev.off()

##sdfm <- subset(dfm, metric=="stretch" & movement==500 & solution!="ours")
##summary(sdfm)
##svg("plots/stretch-uniform-waypoint.svg")
##ggplot(sdfm, aes(topology, mean, fill=interaction(solution, pattern))) +
##  geom_bar(stat="identity", position=position_dodge(0.9)) + #position="dodge") +
##  ylim(0, 3) +
##  xlab("Rocketfuel AS topology") + 
##  ylab("Average path stretch") +
##  geom_errorbar(aes(ymax=max, ymin=min), position=position_dodge(0.9)) +
##  theme_custom_br
##dev.off()


# Evolution of stretch with movement for ours and ours-in

##for (as in AS) {
##    sdfm <- subset(dfm, metric=="stretch" & topology==as & (solution=="ours" | solution=="ours-in"))
##    g <- ggplot(sdfm, aes(movement, mean, color=solution, group=interaction(solution, pattern), linetype=pattern)) +
##        geom_line(size=0.5) +
##        xlim(0, 50) +
##        theme_custom
##    svg(paste("plots/stretch-impact-in-", as, ".svg", sep=""))
##    print(g)
##    dev.off()
##}

for (as in AS) {
    sdfm <- subset(dfm, metric=="stretch" & pattern=="waypoint" & topology==as ) # | solution=="map-me-in"))
    g <- ggplot(sdfm, aes(movement, mean, color=solution, group=solution, shape=solution)) +
        geom_line(size=1.0) +
        geom_point(size=6) +
        xlim(0, 50) +
        scale_shape_manual(values = c(1, 2, 3, 4, 5)) +
        scale_colour_manual(values = colors) +
        xlab("Number of handovers") +
        ylab("Average path stretch") +
    theme_custom_br +
    theme(legend.title=element_blank(),  axis.text.x = element_text(size=16))
    svg(paste("plots/stretch-evolution-waypoint-", as, ".svg", sep=""))
    print(g)
    dev.off()
}

for (as in AS) {
    sdfm <- subset(dfm, metric=="stretch" & pattern=="uniform" & topology==as ) # | solution=="map-me-in"))
    g <- ggplot(sdfm, aes(movement, mean, color=solution, group=solution, shape=solution)) +
        geom_line(size=1.0) +
        geom_point(size=6) +
        xlim(0, 50) +
        scale_shape_manual(values = c(1, 2, 3, 4, 5)) +
        scale_colour_manual(values = colors) +
        xlab("Number of handovers") +
        ylab("Average path stretch") +
    theme_custom_br +
    theme(legend.title=element_blank(),  axis.text.x = element_text(size=16))
    svg(paste("plots/stretch-evolution-uniform-", as, ".svg", sep=""))
    print(g)
    dev.off()
}
