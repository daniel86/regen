
#ifndef __DOXYGEN_H_
#define __DOXYGEN_H_

/// @mainpage ogle
///
/// @section intro Introduction
/// ogle is a portable OpenGL library written in C++.
/// The purpose of the library is to help creating
/// real-time rendering software.
///
/// A layer that abstracts most of the OpenGL 4.0 API is used.
/// All related types are defined in \link gl-types \endlink .
/// ogle::RenderState wraps server side states in stacks so that it is easy
/// possible to reset states to their initial condition.
/// Shader loading is done using CPU-side GLSL pre-processors.
/// First of all the pre-processors allow using include directives
/// which are resolved using GLSW. This allows nicer management of shader
/// resources. The pre-processors additionally support various other
/// code manipulations (generating IO code, manipulation IO types, for directives,
/// blending out undefined code).
///
/// On-top of that a hierarchical state layer is implemented.
/// Each ogle::State can be combined with other states and added to a ogle::StateNode.
/// ogle::StateNode's can have one parent and multiple child nodes.
/// When the hierarchical structure is traversed each ogle::State
/// is enabled and disabled individually. ogle::State's usually modify
/// the current ogle::RenderState, provide input data to shader programs,
/// define configurations used for loading shaders or act as container for other states.
///
/// @section features Feature List
/// Here you find a brief list of supported features in this library.
///
/// - 3D audio using OpenAL
/// - Video loading using libav
///     - audio streaming to OpenAL
/// - Image loading using DevIL
/// - Noise textures using libnoise
/// - Font loading using FreeType
///     - displayed using texture mapped text
/// - Shader pre-processing
///     - input modification (constant, uniform, attribute, instanced attribute)
///     - support for 'include' and 'for' directive
/// - Assimp model loading
///     - bone animation loading
/// - Particle engine using transform feedback
/// - Dynamic sky scattering
/// - Instancing
///     - using instanced attributes
///     - instanced bone animation using bone TBO
/// - GPU mesh animation using transform feedback
/// - Deferred and Direct shading
/// - Ambient Occlusion
/// - Volumetric fog
/// - Simple raycasting volume renderer
/// - Geometry picking
///
/// @section deps Dependency List
/// Following you can find a list of libraries that must be installed in order
/// to compile ogle.
/// - OpenGL
/// - Boost >1.5
/// - libnoise
/// - assimp
/// - DevIL
/// - FreeType
/// - libav
/// - OpenAL
///
/// In order to compile the test applications you will also need to install
/// the following list of libraries:
/// - QtCore
/// - QtGui
/// - QtOpenGL
///
/// @section contact Contact
/// daniel@orgizm.net
///

/// @dir algebra
/// \brief Frequently used algorithms for 3D geometry.
/// @dir animations
/// \brief Collection of Animation implementations.
/// @dir av
/// \brief Interface for OpenAL and libav.
/// @dir gl-types
/// \brief Interface to OpenGL API.
/// @dir meshes
/// \brief Mesh implementations.
/// @dir shading
/// \brief Lights and shadows.
/// @dir states
/// \brief Collection of State implementations.
/// @dir textures
/// \brief Texture loading.
/// @dir utility
/// \brief Miscellaneous types, defines and algorithms.

#endif /* __DOXYGEN_H_ */
