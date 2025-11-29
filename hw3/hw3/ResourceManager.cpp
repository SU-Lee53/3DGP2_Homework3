#include "stdafx.h"
#include "ResourceManager.h"

std::shared_ptr<GameObject> ResourceManager::AddGameObject(const std::string& strObjKey, std::shared_ptr<GameObject> pObject)
{
	if (m_pGameObjects.contains(strObjKey)) return m_pGameObjects[strObjKey];

	m_pGameObjects.insert({ strObjKey, pObject });
	return pObject;
}

std::shared_ptr<GameObject> ResourceManager::GetGameObject(const std::string& strObjKey)
{
	auto it = m_pGameObjects.find(strObjKey);
	if (it == m_pGameObjects.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<GameObject> ResourceManager::CopyGameObject(const std::string& strObjKey)
{
	auto it = m_pGameObjects.find(strObjKey);
	if (it == m_pGameObjects.end()) {
		return nullptr;
	}

	return GameObject::CopyObject(*it->second);
}
