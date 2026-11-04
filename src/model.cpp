#include "app.hpp"

#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

// OBJ indices start at 1, and negative ones count back from the end of the list.
static size_t objIndex(int index, size_t count) {
    return index > 0 ? index - 1 : count + index;
}

void App::loadModel(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open model: " + path);
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::map<std::pair<int, int>, uint32_t> seen;   // (position, texCoord) -> vertex index

    // Turns one "v", "v/vt", "v//vn" or "v/vt/vn" corner into a vertex index.
    auto addCorner = [&](const std::string& corner) {
        size_t slash = corner.find('/');
        int v = std::stoi(corner);
        int vt = 0;
        if (slash != std::string::npos && slash + 1 < corner.size() && corner[slash + 1] != '/') {
            vt = std::stoi(corner.substr(slash + 1));
        }

        auto [entry, isNew] = seen.try_emplace({v, vt}, static_cast<uint32_t>(vertices.size()));
        if (isNew) {
            Vertex vertex{};
            vertex.pos = positions.at(objIndex(v, positions.size()));
            if (vt != 0) {
                glm::vec2 uv = texCoords.at(objIndex(vt, texCoords.size()));
                vertex.texCoord = {uv.x, 1.0f - uv.y};   // OBJ's V goes up, Vulkan's goes down
            }
            vertices.push_back(vertex);
        }
        return entry->second;
    };

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string type;
        in >> type;

        if (type == "v") {
            glm::vec3 p;
            in >> p.x >> p.y >> p.z;
            positions.push_back(p);
            
        } else if (type == "vt") {
            glm::vec2 t;
            in >> t.x >> t.y;
            texCoords.push_back(t);
        } else if (type == "f") {
            std::vector<uint32_t> face;
            std::string corner;
            while (in >> corner) {
                face.push_back(addCorner(corner));
            }
            // Faces can have more than 3 corners, so split them into a fan of triangles.
            for (size_t i = 1; i + 1 < face.size(); i++) {
                indices.insert(indices.end(), {face[0], face[i], face[i + 1]});
            }
            for (size_t i = 0; i < face.size(); i++) {
                glm::vec3 p0 = vertices[face[i]].pos;
                glm::vec3 p1 = vertices[face[(i + 1) % face.size()]].pos;
                glm::vec3 p2 = vertices[face[(i + 2) % face.size()]].pos;
                glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
                vertices.at(face[i]).normal += n; // add normals (need sum into a per-position array instead)
            }
        }
    }

    if (indices.empty()) {
        throw std::runtime_error("model has no faces: " + path);
    }
}
