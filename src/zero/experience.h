#ifndef EXPERIENCE_H
#define EXPERIENCE_H

#include <vector>

#include <torch/torch.h>

class ExperienceCollector {
public:
  std::vector<torch::Tensor> states;
  std::vector<torch::Tensor> visit_counts;
  std::vector<float> rewards;

private:
  std::vector<torch::Tensor> current_episode_states;
  std::vector<torch::Tensor> current_episode_visit_counts;
  
public:

  void begin_episode() {
    current_episode_states.clear();
    current_episode_visit_counts.clear();
  }

  void record_decision(torch::Tensor state, torch::Tensor visit_counts) {
    current_episode_states.push_back(state);
    current_episode_visit_counts.push_back(visit_counts);
  }

  void complete_episode(float reward) {
    states.insert(states.end(), current_episode_states.begin(), current_episode_states.end());
    visit_counts.insert(visit_counts.end(), current_episode_visit_counts.begin(), current_episode_visit_counts.end());
    rewards.insert(rewards.end(), current_episode_states.size(), reward);

    // Clear current episode containers.
    current_episode_states.clear();
    current_episode_visit_counts.clear();
  }

};



#endif // EXPERIENCE_H
