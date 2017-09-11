#!/usr/bin/env Rscript

# Plots to do:
#
# - streaming performance (loss and delay)
#    . vs speed
#    . vs load
#
# - signaling overhead (num messages)
#    . vs load (BOF, maybe because no congestion, let's see)
#    . vs speed (impact of lost packets)
#
# > explaining metrics
#
# - l3 handover delay (need to understand zeng's file format)
#
# - link load (use rate-trace.txt + map links)
#
# - stretch : OK with hop_count
# 

# Plot streaming flow statistics
# Sample usage (yes, truncate path after -m parameter)
# ./scripts/plotall.R ./data/r0.7-K1000-C0-s3000-a0.8-d1000-e10-p0-b20-c50-m

source('scripts/plot_helpers.R')

# Seems needed... why ?
if (exists("dfm"))
  rm(dfm)

scenario_path <- commandArgs(TRUE)[1]
scenario <- basename(scenario_path)
cat(paste("I: Loading data for scenario", scenario, "...\n"));

# 1) User performance metrics

if ((scenario == "load") | (scenario == "speed") | (scenario == "speed5") | (scenario == "speed-tree") | (scenario == "topologies") | (scenario == "topologies-real") | (scenario == "markov-uniform")) {
  dfm <- build_data_frame(scenario_path, "flow_stats_streaming*", debug = FALSE)
    print(summary(dfm))
  dfm$loss_rate = dfm$loss_rate * 100
  dfm$delay = dfm$delay * 1000

  if ((scenario == "speed") | (scenario == "speed5") | (scenario == "speed-tree")) {
    a <- with(dfm, aggregate(dfm[, c("elapsed", "delay", "loss_rate", "hop_count")], list(speed=speed, Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
    print(summary(a))
    plots <- list(
      list("speed", "Speed [m/s]", "delay", "Average packet delay [ms]"),
      list("speed", "Speed [m/s]", "loss_rate", "Average packet loss [%]"),
      list("speed", "Speed [m/s]", "hop_count", "Average hop count")
    )
    generate_plots(scenario_path, a, "line", FALSE, "br", plots)
  } else if ((scenario == "topologies") | (scenario == "topologies-real")| (scenario == "markov-uniform") | (scenario == "markov-rwp") | (scenario == "markov-rw")) {
    a <- with(dfm, aggregate(dfm[, c("elapsed", "delay", "loss_rate", "hop_count")], list(topology=topology, Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
    cat("----------pol----------------\n");
    print(summary(a))
    plots <- list(
      list("topology", "Topology", "delay", "Average packet delay [ms]"),
      list("topology", "Topology", "loss_rate", "Average packet loss [%]"),
      list("topology", "Topology", "hop_count", "Average hop count")
    )
    generate_plots(scenario_path, a, "bar", FALSE, "tr", plots)
  } else if (scenario == "load") {
    a <- with(dfm, aggregate(dfm[, c("elapsed", "delay", "loss_rate", "hop_count")], list(num_cp_couples=num_cp_couples, Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
    plots <- list(
      list("num_cp_couples", "Demand in # of C/P couples", "delay", "Average packet delay [ms]"),
      list("num_cp_couples", "Demand in # of C/P couples", "loss_rate", "Average packet loss [%]"),
      list("num_cp_couples", "Demand in # of C/P couples", "hop_count", "Average hop count")
    )
    generate_plots(scenario_path, a, "line", FALSE, "br", plots)
  }

}

# 2) Network performance metrics
#
# 2a) Signalling overhead

rm(dfm)

dfm <- build_data_frame(scenario_path, "signaling_overhead")
# XXX Until this is fixed in the code, for anchor, we need to add 1 hop
# from the overhead, since it includes the packet forwarded to the application

# signaling_overhead : time node_id prefix seq ttl
#summary(dfm)

GR_NUM_MESSAGES <- 29
number_ticks <- function(n) {function(limits) pretty(limits, n)}

if ((scenario == "speed") | (scenario == "speed5")){
  a <- merge(aggregate(ttl ~ speed+Mobility+run, dfm, function(x) sum(!is.na(x))), aggregate(seq ~ speed+Mobility+run, dfm, max))
  b <- within(a, avg_num_messages <- ttl / seq)
  c <- aggregate(avg_num_messages ~ speed + Mobility, b, mean_sd)

# XXX Need fix this removes anchor values otherwise
  c$avg_num_messages[c$Mobility == "AB"] <- 4 #c$avg_num_messages[c$Mobility == "AB"] + 1

  # Fake GR results to have it in legend
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, speed=1, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, speed=2, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, speed=5, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, speed=10, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, speed=15, Mobility="GR"))

  summary(dfm)

  g <- get_plot(scenario, c, "line", FALSE, "tl", "speed", "Speed [m/s]", "avg_num_messages", "Number of messages")
  g <- g + coord_cartesian(ylim=c(0,GR_NUM_MESSAGES)) + scale_y_continuous(breaks=number_ticks(GR_NUM_MESSAGES/5))
  make_svg(scenario, g, "avg_num_messages")

} else if (scenario == "load") {
  a <- merge(aggregate(ttl ~ num_cp_couples+Mobility+run+prefix, dfm, function(x) sum(!is.na(x))), aggregate(seq ~ num_cp_couples+Mobility+run+prefix, dfm, max))
  b <- within(a, avg_num_messages <- ttl / seq)
  c <- aggregate(avg_num_messages ~ num_cp_couples + Mobility, b, mean_sd)

  # Fake GR results to have it in legend
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=1, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=2, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=3, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=10, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=15, Mobility="GR"))
  c <- rbind(c, data.frame(avg_num_messages=GR_NUM_MESSAGES, num_cp_couples=20, Mobility="GR"))

  g <- get_plot(scenario, c, "line", FALSE, "br", "num_cp_couples", "Demand [# of C/P couples]", "avg_num_messages", "Number of messages")
  g <- g + coord_cartesian(ylim=c(0,GR_NUM_MESSAGES)) + scale_y_continuous(by=5)
  make_svg(scenario, g, "avg_num_messages")

} else if ((scenario == "baseline") | (scenario == "baseline-100") | (scenario == "baseline-2000")) {

  dfm$node_class <- factor(mapply(get_node_class, dfm$node_id), levels=c("Access", "Edge", "Backhaul", "Core", "Mobile"))


  # Don't show Mobile nodes, and suppress Access and Backhaul components from
  # Anchor, that are forwarding overhead, and not processing overhead
  # (in fact, this should not be accounted for in the forwarder)

  #  we need node class for kite since the seqno is there
  #dfm <- subset(dfm, (node_class != "Mobile") & ! ((Mobility == "AB") & (node_class != "Core")))
  dfm <- subset(dfm, ! ((Mobility == "AB") & (node_class != "Core")))
  dfm2 <- build_data_frame(scenario_path, "processing_overhead", debug=TRUE)
  dfm2$node_class <- factor(mapply(get_node_class, dfm2$node_id), levels=c("Access", "Edge", "Backhaul", "Core", "Mobile"))

  numh <- aggregate(seq ~ Mobility+num_cp_couples+prefix+run, dfm, max)
  print(summary(numh))
  pnod <- aggregate(ttl ~ Mobility+num_cp_couples+prefix+run+node_class+node_id, dfm2, function (x) sum(!is.na(x)))
  print(summary(pnod))
  tot <- merge(pnod, numh)
  tot <- within(tot, ratio <- ttl / seq) 
  x <- aggregate(ratio ~ Mobility+node_class, tot, mean_sd)
  

  # We plot processing, not sending, so everything should be shifted
#  x$node_class[x$node_class == "Backhaul" & x$Mobility == "TB"] <- "Core"
#  x$node_class[x$node_class == "Edge" & x$Mobility == "TB"] <- "Backhaul"
#  x$node_class[x$node_class == "Access" & x$Mobility == "TB"] <- "Edge"
#  x$node_class[x$node_class == "Mobile" & x$Mobility == "TB"] <- "Access"

  x <- subset(x, node_class != "Mobile")

  x <- rbind(x, data.frame(Mobility="GR", node_class="Access", ratio=1))
  x <- rbind(x, data.frame(Mobility="GR", node_class="Backhaul", ratio=1))
  x <- rbind(x, data.frame(Mobility="GR", node_class="Edge", ratio=1))
  x <- rbind(x, data.frame(Mobility="GR", node_class="Core", ratio=1))
  x <- rbind(x, data.frame(Mobility="MapMe-IU", node_class="Core", ratio=0))
  x <- rbind(x, data.frame(Mobility="MapMe", node_class="Core", ratio=0))

  x <- rbind(x, data.frame(Mobility="AB", node_class="Access", ratio=0))
  x <- rbind(x, data.frame(Mobility="AB", node_class="Edge", ratio=0))
  x <- rbind(x, data.frame(Mobility="AB", node_class="Backhaul", ratio=0))
  x <- rbind(x, data.frame(Mobility="AB", node_class="Core", ratio=1))

  plots <- list(
    list("node_class", "Node class", "ratio", "Signaling overhead [pkts/handover]")
  )

  generate_plots(scenario_path, x, "bar", FALSE, "tl", plots)

  # L3 delay

  rm(dfm)
  dfm <- build_data_frame(scenario_path, "producer_handover_stats-*")
  dfm$l3_handoff_time <- dfm$l3_handoff_time * 1000

  plots <- list(
    list("", "", "l3_handoff_time", "Layer 3 handoff latency [ms]")
  )

  generate_plots(scenario_path, dfm, "cdf", FALSE, "br", plots)
}
