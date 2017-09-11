#!/usr/bin/Rscript

source('scripts/plot_helpers.R')

if (exists("dfm"))
  rm(dfm)

scenario <- commandArgs(TRUE)[1]
cat(paste("I: Loading data for scenario", scenario, "...\n"))

# rate-trace.txt : Time  Node  FaceId  FaceDescr OtherNode Type  Packets Kilobytes PacketRaw KilobytesRaw
# /!\ Huge file so very slow... we might need a better way to deal with such traces
# /!\ ... even summary() is very slow

callback <- function(temp_dfm)
{
  temp_dfm <- subset(temp_dfm, temp_dfm$Type == "OutData")
  return(temp_dfm)
}

fn <- paste(".", scenario, "-rate-trace.rds", sep="")
if (!file.exists(fn)) {
  dfm <- build_data_frame(scenario, "rate-trace.txt", callback, TRUE)
  dfm$link_id <- as.factor(mapply(get_link_id, dfm$Node, dfm$OtherNode))
  dfm$link_class <- factor(mapply(get_link_class, dfm$Node, dfm$OtherNode), levels=c("Access", "Mesh Access", "Edge", "Mesh Edge", "Backhaul", "Mesh Backhaul", "Core"))
  saveRDS(dfm, file=fn)

} else {
  dfm <- readRDS(fn)
# TEMP FIX
  dfm$link_class <- factor(mapply(get_link_class, dfm$Node, dfm$OtherNode), levels=c("Access", "Edge", "Backhaul", "Core"))  
  cat(paste("Using cached data frame from", fn, "\n"))
}

# Before aggregating per link class, we need to sum uplink and downlink with same link_id
a <- aggregate(KilobytesRaw ~ Mobility+run+speed+Time+num_cp_couples+link_class+link_id, dfm, sum)

# Obtain link class and compute average by class
b <- aggregate(KilobytesRaw ~ Mobility+link_class, a, mean_sd)

b$KilobytesRaw <- b$KilobytesRaw * 8 / 1000

# Fake records to get all bar in each class...
b <- rbind(b, data.frame(Mobility="GR", link_class="Core", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="MapMe", link_class="Core", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="MapMe-IU", link_class="Core", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="GR", link_class="Access", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="AB", link_class="Access", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="TB", link_class="Access", KilobytesRaw=0))
b <- rbind(b, data.frame(Mobility="MapMe-IU", link_class="Access", KilobytesRaw=0))

# TO REALLY GET THE ORDER RIGHT AFTER THE RBIND
b$link_class <- factor(b$link_class, levels=c("Access", "Edge", "Backhaul", "Core"))

generate_plot(scenario, b, "bar", FALSE, "tl", "link_class", "Link class", "KilobytesRaw", "Link utilization [Mb/s]")

