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
    // Unsqueeze so that we get expected shape when concatenating
    current_episode_states.push_back(state.unsqueeze(0));
    current_episode_visit_counts.push_back(visit_counts.unsqueeze(0));
  }

  void complete_episode(float reward) {
    states.insert(states.end(), current_episode_states.begin(), current_episode_states.end());
    visit_counts.insert(visit_counts.end(), current_episode_visit_counts.begin(), current_episode_visit_counts.end());
    rewards.insert(rewards.end(), current_episode_states.size(), reward);

    // Clear current episode containers.
    current_episode_states.clear();
    current_episode_visit_counts.clear();
  }

  // Instead of using experience buffer, simpler to add a combine_experience
  // method here.

  void serialize(const std::string path) {
    auto states_tensor = torch::cat(states);
    auto visit_counts_tensor = torch::cat(visit_counts);

    auto rewards_tensor = torch::from_blob(rewards.data(),
                                           {static_cast<int64_t>(rewards.size())}).to(torch::kFloat32);

    auto tensors = torch::TensorList({states_tensor, visit_counts_tensor, rewards_tensor});

    auto bytes = torch::pickle_save(tensors);
    std::ofstream fout(path, std::ios::out | std::ios::binary);
    fout.write(bytes.data(), bytes.size());
    fout.close();
  }

};



#endif // EXPERIENCE_H
