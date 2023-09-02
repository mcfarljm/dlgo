#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <optional>

#include "scoring.h"


class CollectRegion {

  PointSet visited_points;

  static constexpr int deltas[][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};

public:

  // Find the contiguous section of a board containing a point. Also identify
  // all the boundary points.
  std::pair<PointSet, std::unordered_set<std::optional<Player>>>
  collect_region(Point start_pos, ConstBoardPtr board) {
    if (visited_points.find(start_pos) != visited_points.end())
      return std::make_pair<PointSet, std::unordered_set<std::optional<Player>>>({},{});

    PointSet all_points;
    std::unordered_set<std::optional<Player>> all_borders;

    all_points.insert(start_pos);
    visited_points.insert(start_pos);

    auto here = board->get(start_pos);

    for (auto &[delta_r, delta_c] : deltas) {
      auto next_p = Point(start_pos.row + delta_r, start_pos.col + delta_c);
      if (! board->is_on_grid(next_p))
        continue;
      auto neighbor = board->get(next_p);
      if (neighbor == here) {
        auto [points, borders] = collect_region(next_p, board);
        for (const auto& point : points)
          all_points.insert(point);
        for (const auto& border : borders)
          all_borders.insert(border);
      } else
        all_borders.insert(neighbor);
    }

    return std::make_pair(all_points, all_borders);
  }

};


Territory::Territory(std::unordered_map<Point, TerritoryStatus, PointHash> territory_map) {
  for (const auto& [point, status] : territory_map) {
    switch (status) {
    case TerritoryStatus::black_stone:
      ++num_black_stones;
      break;
    case TerritoryStatus::white_stone:
      ++num_white_stones;
      break;
    case TerritoryStatus::black_territory:
      ++num_black_territory;
      break;
    case TerritoryStatus::white_territory:
      ++num_white_territory;
      break;
    case TerritoryStatus::dame:
      ++num_dame;
      dame_points.insert(point);
      break;
    }
  }
}


Territory Territory::evaluate_territory(ConstBoardPtr board) {
  std::unordered_map<Point, TerritoryStatus, PointHash> territory_map;
  for (int r=1; r <= board->num_rows; ++r) {
    for (int c=1; c <= board->num_cols; ++c) {
      auto p = Point(r, c);
      if (territory_map.find(p) != territory_map.end())
        continue;
      auto stone = board->get(p);
      if (stone)
        territory_map[p] = stone.value() == Player::white ? TerritoryStatus::white_stone : TerritoryStatus::black_stone;
      else {
        auto collector = CollectRegion();
        auto [group, neighbors] = collector.collect_region(p, board);
        TerritoryStatus fill_with;
        if (neighbors.size() == 1) {
          auto neighbor_stone = *neighbors.begin();
          assert(neighbor_stone);
          fill_with = neighbor_stone.value() == Player::black ? TerritoryStatus::black_territory : TerritoryStatus::white_territory;
        }
        else
          fill_with = TerritoryStatus::dame;
        for (const auto& pos : group) {
          territory_map[pos] = fill_with;
        }
      }
    }
  }
      
  return Territory(std::move(territory_map));
}
