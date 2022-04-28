aux_source_directory(./src DIR_APP_SRC)
aux_source_directory(./incc DIR_APP_SRC)

include_directories(
	./inc
)

add_library(avtp_qanthink ${DIR_APP_SRC})

target_link_libraries(avtp_qanthink)

