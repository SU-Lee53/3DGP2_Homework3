#pragma once
#include "GameObject.h"

class ResourceManager {
public:
	std::shared_ptr<GameObject> AddGameObject(const std::string& strObjKey, std::shared_ptr<GameObject> pObject);
	std::shared_ptr<GameObject> GetGameObject(const std::string& strObjKey);
	std::shared_ptr<GameObject> CopyGameObject(const std::string& strObjKey);


private:
	std::unordered_map<std::string, std::shared_ptr<GameObject>> m_pGameObjects;

};

