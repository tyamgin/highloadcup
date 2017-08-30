require(ggplot2)

path = '/home/tyamgin/CLionProjects/hlcupdocs-master/logs/2017-08-26_11-36-15.ULYuxR/phout_gotUm6.log'

conn = file(path, open="r")
linn = readLines(conn)
close(conn)

processLine = function (line) {
  parts = strsplit(line, '\t', fixed=T)[[1]]
  query = strsplit(parts[1], '.', fixed=T)[[1]]

  c(query[1], query[2], parts[2], parts[3], parts[4], parts[5], parts[6], parts[7])
}

X = matrix(sapply(linn, processLine), nrow=length(linn), byrow=T)
X[, 1] = as.integer(X[, 1])
X[, 4] = as.integer(X[, 4])
X[, 5] = as.integer(X[, 5])
X[, 6] = as.integer(X[, 6])
X[, 7] = as.integer(X[, 7])
X[, 8] = as.integer(X[, 8])
colnames(X) <- c('timestamp', 'entity_id', 'type', 'sum', 'connect', 'write', 'latency', 'read')

X = data.frame(
  timestamp = as.integer(X[, 1]),
  entity_id = X[, 2],
  type = X[, 3],
  sum = as.integer(X[, 4]) / 1e6,
  connect = as.integer(X[, 5]) / 1e6,
  write = as.integer(X[, 6]) / 1e6,
  latency = as.integer(X[, 7]) / 1e6,
  read = as.integer(X[, 8]) / 1e6
)

cat('Summary:\n')
cat(paste0('sum ', sum(X$sum), ' sec'))

sum_aggr = aggregate(X$sum, list(X$timestamp), sum)
sum_aggr$color = 1
latency_aggr = aggregate(X$latency, list(X$timestamp), sum)
latency_aggr$color = 2

data = rbind(sum_aggr, latency_aggr)

print(
  ggplot(NULL,  aes(x=data[, 1], y=data[, 2], group=data[,3], color=data[,3])) + geom_point() + geom_line()
)