/*
 * resource-factory.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef RESOURCE_FACTORY_H_
#define RESOURCE_FACTORY_H_

#include <regen/utility/filesystem.h>
#include <regen/utility/string-util.h>
#include <regen/utility/ref-ptr.h>
#include <regen/config.h>

#include <regen/scene/scene-loader.h>

#include <string>
#include <map>

namespace regen {
	namespace scene {
		/**
		 * Creates resources from SceneInputNode attributes.
		 */
		template<class T>
		class ResourceProcessor {
		public:
			/**
			 * Default constructor.
			 * @param category The resource category.
			 */
			explicit ResourceProcessor(const std::string &category)
					: category_(category) {}

			virtual ~ResourceProcessor() = default;

			/**
			 * @return The resource category identifier.
			 */
			const std::string &category() { return category_; }

			/**
			 * @return The resource map.
			 */
			auto &resources() { return resources_; }

			/**
			 * Add a named resource.
			 * @param name The resource name.
			 * @param resource The resource instance.
			 */
			void putResource(const std::string &name, const ref_ptr<T> &resource) { resources_[name] = resource; }

			/**
			 * Obtain a resource.
			 * Create from SceneParser input if the resource was not created yet.
			 * @param parser the SceneParser
			 * @param resourceName The name of the resource.
			 * @return the resource or null.
			 */
			ref_ptr<T> getResource(SceneLoader *parser, const std::string &resourceName) {
				if (resources_.count(resourceName) > 0) {
					return resources_[resourceName];
				} else {
					const ref_ptr<SceneInputNode> &root = parser->getRoot();
					ref_ptr<SceneInputNode> input = root->getFirstChild(category_, resourceName);
					if (input.get()) {
						ref_ptr<T> resource = createResource(parser, *input.get());
						resources_[resourceName] = resource;
						return resource;
					} else {
						return ref_ptr<T>();
					}
				}
			}

			/**
			 * Create a new resource instance using SceneInputNode attributes.
			 * @param parser The SceneParser
			 * @param input The SceneInputNode
			 * @return The created resource or a null reference
			 */
			virtual ref_ptr<T> createResource(
					SceneLoader *parser, SceneInputNode &input) = 0;

		protected:
			std::map<std::string, ref_ptr<T> > resources_;
			std::string category_;
		};
	}
}

#endif /* RESOURCE_FACTORY_H_ */
