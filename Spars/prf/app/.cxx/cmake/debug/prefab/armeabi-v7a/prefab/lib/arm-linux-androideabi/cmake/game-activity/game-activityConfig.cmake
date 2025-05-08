if(NOT TARGET game-activity::game-activity)
add_library(game-activity::game-activity INTERFACE IMPORTED)
set_target_properties(game-activity::game-activity PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/home/richardwu/.gradle/caches/transforms-3/a94f3c1cbd7846ac0ca47ef2b4af96bd/transformed/games-activity-1.1.0/prefab/modules/game-activity/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

