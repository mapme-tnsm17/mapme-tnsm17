#!/usr/bin/env Rscript

source('scripts/plot_helpers.R')

num_cp_couples <- 5
scenario <- commandArgs(TRUE)[1]

cat(paste("I: Loading data for scenario", scenario, "...\n"));

dfm1 <- build_data_frame(scenario, "signaling_overhead")
a1 <- merge(aggregate(ttl ~ speed+Mobility+run+TU, dfm1, function(x) sum(!is.na(x))), aggregate(seq ~ speed+Mobility+run+TU, dfm1, max))
b1 <- within(a1, avg_num_messages <- ttl / seq / num_cp_couples)
c1 <- aggregate(avg_num_messages ~ Mobility+TU, b1, mean_sd)

dfm2 <- build_data_frame(scenario, "flow_stats_streaming*")
dfm2$loss_rate = dfm2$loss_rate * 100
dfm2$delay = dfm2$delay * 1000
# /!\\ XXX Mobility should be optional, mean_sd too. Rework plot script
a2 <- with(dfm2, aggregate(dfm2[, c("elapsed", "delay", "loss_rate", "hop_count")], list(Mobility=Mobility, TU=TU), mean_sd))

fn <- paste(".", scenario, "-rate-trace.rds", sep="")
if (!file.exists(fn)) {
  dfm3 <-  build_data_frame(scenario, "rate-trace.txt", debug=TRUE)

  dfm3$link_id <- as.factor(mapply(get_link_id, dfm3$Node, dfm3$OtherNode))
  dfm3$link_class <- factor(mapply(get_link_class, dfm3$Node, dfm3$OtherNode), levels=c("Access", "Edge", "Backhaul", "Core", "NA"))  
  dfm3 <- subset(dfm3, link_class == "Access")

  saveRDS(dfm3, file=fn)

} else {
  dfm3 <- readRDS(fn)
  cat(paste("Using cached data frame from", fn, "\n"))
}


# Filter out non-X2 links
dfm3$link_class_ext <- factor(mapply(get_link_class_ext, dfm3$Node, dfm3$OtherNode), levels=c("MeshAccess", "MeshEdge", "MeshBackhaul", "CoreBackhaul", "BackhaulEdge", "EdgeAccess", "AccessMobile", "NA"))
summary(dfm3)
dfm3 <- subset(dfm3, link_class_ext == "MeshAccess")

a3 <- aggregate(KilobytesRaw ~ Mobility+TU+run+speed+Time+num_cp_couples+link_class+link_class_ext+link_id, dfm3, sum)
b3 <- aggregate(KilobytesRaw ~ Mobility+TU+link_class_ext, a3, mean_sd)
b3$KilobytesRaw <- b3$KilobytesRaw * 8 / 1000

p1 <- get_plot(scenario, c1, "line", FALSE, "tr", "TU", "TU parameter [s]", "avg_num_messages", "Avg. messages per handover")
p2 <- get_plot(scenario, a2, "line", FALSE, "tr", "TU", "TU parameter [s]", "loss_rate", "Average loss rate")
p3 <- get_plot(scenario, a2, "line", FALSE, "tr", "TU", "TU parameter [s]", "hop_count", "Average hop count")
p4 <- get_plot(scenario, b3, "line", FALSE, "tr", "TU", "TU parameter [s]", "KilobytesRaw", "Average utilization of inter-BS links [Mb/s]")

##### GRID PLOTS

#library(gridExtra)
#
#print("merging plots")
#p <- grid.arrange(p1, p4, ncol=1)
#p <- grid.arrange(p1, p2, p3, ncol=1)

make_svg(scenario, p4, "tradeoff-utilization")


###### OVERLAPPING PLOTS

library(gtable)
library(grid)
library(scales) # pretty_breaks

p1 <- p1 +
    xlim(0, 51) +
    geom_line(colour="#f8756bff", size=DEFAULT_LINE_SIZE) +
    geom_point(colour="#f8756bff", size=DEFAULT_POINT_SIZE) +
    theme(axis.title.y = element_text(colour = "#f8756bff"), legend.position = "none")

p2 <- p4 +
    xlim(0, 51) +
    geom_line(colour="#00b0f5ff", size=DEFAULT_LINE_SIZE) +
    geom_point(colour="#00b0f5ff",size=DEFAULT_POINT_SIZE) +
    theme(axis.title.y = element_text(colour = "#00b0f5ff"), legend.position = "none") %+replace%
    theme(panel.grid.major = element_blank(), panel.grid.minor = element_blank(), panel.background = element_rect(fill = NA))

# Experimental 
grid.newpage()

# extract gtable
g1 <- ggplot_gtable(ggplot_build(p1))
g2 <- ggplot_gtable(ggplot_build(p2))

# overlap the panel of 2nd plot on that of 1st plot
pp <- c(subset(g1$layout, name == "panel", se = t:r))
g <- gtable_add_grob(g1, g2$grobs[[which(g2$layout$name == "panel")]], pp$t, 
        pp$l, pp$b, pp$l)

# axis tweaks
ia <- which(g2$layout$name == "axis-l")
ga <- g2$grobs[[ia]]
ax <- ga$children[[2]]
ax$widths <- rev(ax$widths)
ax$grobs <- rev(ax$grobs)
ax$grobs[[1]]$x <- ax$grobs[[1]]$x - unit(1, "npc") + unit(0.15, "cm")
g <- gtable_add_cols(g, g2$widths[g2$layout[ia, ]$l], length(g$widths) - 1)
g <- gtable_add_grob(g, ax, pp$t, length(g$widths) - 1, pp$b)

make_svg(scenario, grid.draw(g), "tradeoff")
