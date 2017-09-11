library(ggplot2)

t <- read.table('results/cs-trace.txt', col.names=c('time', 'cache', 'key', 'value'))
t$value <- as.numeric(t$value)
t$time <- as.numeric(t$time)
u<-droplevels(t[-which((t$cache!="B1")|(t$key!="CacheHits")),])
ggplot(u, aes(time, value)) + geom_line()

ggplot(droplevels(t[which((t$cache=="B1")&(t$key=="CacheHits")),]), aes(time, value)) + geom_line()
