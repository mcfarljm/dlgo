#include <filesystem>
#include "experience.h"


void ExperienceCollector::serialize_pickle(const std::string path) {
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

namespace {
  void serialize_tensor(const torch::Tensor& tensor, const std::string directory, const std::string name) {
    auto json_path = std::filesystem::path(directory) / (name + ".json");
    auto data_path = std::filesystem::path(directory) / (name + ".dat");

    std::ofstream fout(json_path, std::ios::out);
    auto dim = tensor.dim();
    fout << "{\n  \"data\": \"" << name << ".dat" << "\",\n";
    fout << "  \"shape\": [";
    for (auto i=0; i<dim; ++i) {
      fout << tensor.size(i);
      if (i + 1 < dim)
        fout << ", ";
    }
    fout << "],\n";
  
    fout << "  \"strides\": [";
    for (auto i=0; i<dim; ++i) {
      fout << tensor.stride(i);
      if (i+1 < dim)
        fout << ", ";
    }
    fout << "],\n";
    fout << "}\n";
  
    fout.close();

    fout.open(data_path, std::ios::out | std::ios::binary);
    fout.write(static_cast<char*>(tensor.data_ptr()), sizeof(float) * at::numel(tensor));
    fout.close();
  }
}


void ExperienceCollector::serialize_binary(const std::string path) {
  if (std::filesystem::exists(path)) {
    if (! std::filesystem::is_directory(path))
      throw std::runtime_error("path exists and is not a directory: " + path);
  }
  else
    std::filesystem::create_directory(path);
  
  auto states_tensor = torch::cat(states);
  auto visit_counts_tensor = torch::cat(visit_counts);
  auto rewards_tensor = torch::from_blob(rewards.data(),
                                         {static_cast<int64_t>(rewards.size())}).to(torch::kFloat32);

  serialize_tensor(states_tensor, path, "states");
  serialize_tensor(visit_counts_tensor, path, "visit_counts");
  serialize_tensor(rewards_tensor, path, "rewards");
}
