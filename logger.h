#ifndef HIGHLOAD_LOGGER_H
#define HIGHLOAD_LOGGER_H

#define M_LOG(expr) (std::cout << time(NULL) << "@ " << expr << std::endl)
#define M_ERROR(expr) M_LOG(expr)

#ifdef DEBUG
#define M_DEBUG_LOG(expr) M_LOG(expr)
#else
#define M_DEBUG_LOG(expr)
#endif


#endif //HIGHLOAD_LOGGER_H
