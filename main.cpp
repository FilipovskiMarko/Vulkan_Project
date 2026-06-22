// #define VK_USE_PLATFORM_WIN32_KHR
// #define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;


const std::vector<char const*> validationLayers = {
  "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		return {{{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
						 {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
						 {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)}}};
	}
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

class HelloTriangleApplication {
public:
  void run() {
    initVulkan();
    mainLoop();
    cleanup();
  }
private:
  GLFWwindow* window;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;
  vk::raii::Queue graphicsQueue = nullptr;
	uint32_t queueIndex = ~0;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  std::vector<vk::raii::ImageView> swapChainImageViews;
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;
	vk::raii::CommandPool commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	vk::raii::Buffer vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	vk::raii::Buffer indexBuffer = nullptr;
	vk::raii::DeviceMemory indexBufferMemory = nullptr;
	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;



	std::vector<vk::raii::Buffer>       uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
	std::vector<void *>                 uniformBuffersMapped;

	uint32_t frameIndex = 0;
	bool framebufferResized = false;

	vk::raii::Image        textureImage       = nullptr;
	vk::raii::DeviceMemory textureImageMemory = nullptr;
	vk::raii::ImageView			 textureImageView = nullptr;
	vk::raii::Sampler			 textureSampler			= nullptr;


	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;



  vk::Extent2D swapChainExtent;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;

  std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  	glfwSetWindowUserPointer(window, this);
  	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
  {
  	auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;

  }

  void initVulkan() {
    initWindow();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
  	createDescriptorSetLayout();
    createGraphicsPipeline();
  	createCommandPool();
  	createTextureImage();
  	createTextureImageView();
  	createTextureSampler();
  	createVertexBuffer();
  	createIndexBuffer();
  	createUniformBuffers();
  	createDescriptorPool();
  	createDescriptorSets();
  	createCommandBuffers();
  	createSyncObjects();
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    	drawFrame();
    }

  	device.waitIdle();
  }

  void cleanup() {
		cleanupSwapChain();

    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void createSurface() {
    VkSurfaceKHR       _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
      throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
  }

  void createInstance() {
    constexpr vk::ApplicationInfo appInfo {.pApplicationName =  "Hello Triangle",
                                          .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .pEngineName = "No Engine",
                                          .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                          .apiVersion = vk::ApiVersion14};
    // EXTENSIONS CHECK
    // Get the required extensions.
    auto requiredExtensions = getRequiredInstanceExtensions();
    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt =
        std::ranges::find_if(requiredExtensions,
                             [&extensionProperties](auto const &requiredExtension) {
                               return std::ranges::none_of(extensionProperties,
                                                           [requiredExtension](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
                             });
    if (unsupportedPropertyIt != requiredExtensions.end())
    {
      throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

    // VALIDATION LAYERS CHECK
    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers)
    {
      requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
                                                   [&layerProperties](auto const &requiredLayer) {
                                                     return std::ranges::none_of(layerProperties,
                                                                                 [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
                                                   });
    if (unsupportedLayerIt != requiredLayers.end())
    {
      throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    vk::InstanceCreateInfo createInfo{
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
      .ppEnabledLayerNames = requiredLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
      .ppEnabledExtensionNames = requiredExtensions.data(),};

    instance = vk::raii::Instance(context, createInfo);
  }

  std::vector<const char*> getRequiredInstanceExtensions()
  {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
  }

  void pickPhysicalDevice() {
    auto physicalDevices = instance.enumeratePhysicalDevices();

    if (physicalDevices.empty())
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    for (auto const &physicalDevice : physicalDevices) {
      if (isDeviceSuitable(physicalDevice)) {
        this->physicalDevice = physicalDevice;
        break;
      }
    }
    if (physicalDevice == nullptr) {
      throw std::runtime_error( "failed to find a suitable GPU!" );
    }
  }

  bool isDeviceSuitable( vk::raii::PhysicalDevice const & physicalDevice ) {
  // Check if the physicalDevice supports the Vulkan 1.3 API version
  bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

  // Check if any of the queue families support graphics operations
  auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics = std::ranges::any_of( queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } );

  // Check if all required physicalDevice extensions are available
  auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions =
    std::ranges::all_of( requiredDeviceExtension,
                         [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
                         {
                           return std::ranges::any_of( availableDeviceExtensions,
                                                       [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                                       { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
                         } );

  // Check if the physicalDevice supports the required features (shader draw parameters, dynamic rendering and extended dynamic state)
  auto features                 = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                                       vk::PhysicalDeviceVulkan13Features,
                                                                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
  	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
																			features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
																			features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
																			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

  // Return true if the physicalDevice meets all the criteria
  return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

  void createLogicalDevice() {
		// find the index of the first queue family that supports graphics
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

		// get the first index into queueFamilyProperties which supports graphics
		auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const &qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
		assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");

		auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

		// query for Vulkan 1.3 features
  	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
  		{.features = {.samplerAnisotropy = true } },            // vk::PhysicalDeviceFeatures2
			{.synchronization2 = true, .dynamicRendering = true },  // vk::PhysicalDeviceVulkan13Features
			{.extendedDynamicState = true }                         // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
  	};

		// create a Device
		float                     queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
		vk::DeviceCreateInfo      deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		                                           .queueCreateInfoCount    = 1,
		                                           .pQueueCreateInfos       = &deviceQueueCreateInfo,
		                                           .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtension.size()),
		                                           .ppEnabledExtensionNames = requiredDeviceExtension.data()};

		device        = vk::raii::Device(physicalDevice, deviceCreateInfo);
		graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
  	queueIndex = graphicsIndex;
	}

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) {
    assert(!availableFormats.empty());

    const auto formatIt = std::ranges::find_if(
      availableFormats,
      [](const auto &format){return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });

    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
  }

  vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](const auto presentMode){return presentMode == vk::PresentModeKHR::eFifo;}));
    return std::ranges::any_of(availablePresentModes,
      [](const vk::PresentModeKHR value) {return value == vk::PresentModeKHR::eMailbox; } ) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {
      std::clamp<uint8_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      std::clamp<uint8_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
  }

  uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
      minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
  }

  void createSwapChain() {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
    swapChainExtent                                = chooseSwapExtent(surfaceCapabilities);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapChainSurfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    auto availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{.surface          = *surface,
                                               .minImageCount    = minImageCount,
                                               .imageFormat      = swapChainSurfaceFormat.format,
                                               .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
                                               .imageExtent      = swapChainExtent,
                                               .imageArrayLayers = 1,
                                               .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
                                               .imageSharingMode = vk::SharingMode::eExclusive,
                                               .preTransform     = surfaceCapabilities.currentTransform,
                                               .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                               .presentMode      = chooseSwapPresentMode(availablePresentModes),
                                               .clipped          = true};

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
    }

	void createImageViews()
  {
  	assert(swapChainImageViews.empty());

  	swapChainImageViews.reserve(swapChainImages.size());
  	for ( auto &image: swapChainImages )
  	{
  		swapChainImageViews.emplace_back(createImageView(image, swapChainSurfaceFormat.format));
  	}
  }

	void createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(readFile("/home/filip/Programs/CLion_Projects/KG_Vulkan/shaders/slang.spv"));

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


  	auto                                     bindingDescription    = Vertex::getBindingDescription();
  	auto                                     attributeDescriptions = Vertex::getAttributeDescriptions();
  	vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{.vertexBindingDescriptionCount   = 1,
																														 .pVertexBindingDescriptions      = &bindingDescription,
																														 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
																														 .pVertexAttributeDescriptions    = attributeDescriptions.data()};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
		vk::PipelineViewportStateCreateInfo      viewportState{.viewportCount = 1, .scissorCount = 1};

		vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
		                                                    .rasterizerDiscardEnable = vk::False,
		                                                    .polygonMode             = vk::PolygonMode::eFill,
		                                                    .cullMode                = vk::CullModeFlagBits::eBack,
		                                                    .frontFace               = vk::FrontFace::eCounterClockwise,
		                                                    .depthBiasEnable         = vk::False,
		                                                    .lineWidth               = 1.0f};

		vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable    = vk::False,
		    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		vk::PipelineColorBlendStateCreateInfo colorBlending{
		    .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

		std::vector<vk::DynamicState>      dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0 };
		pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		    {.stageCount          = 2,
		     .pStages             = shaderStages,
		     .pVertexInputState   = &vertexInputInfo,
		     .pInputAssemblyState = &inputAssembly,
		     .pViewportState      = &viewportState,
		     .pRasterizationState = &rasterizer,
		     .pMultisampleState   = &multisampling,
		     .pColorBlendState    = &colorBlending,
		     .pDynamicState       = &dynamicState,
		     .layout              = pipelineLayout,
		     .renderPass          = nullptr},
		    {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

		graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

  [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>&code) const {
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{.codeSize = code.size() * sizeof(char),
                                                      .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    vk::raii::ShaderModule shaderModule{device , shaderModuleCreateInfo};

    return shaderModule;
  }

	static std::vector<char> readFile(const std::string &filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			std::cout << filename << std::endl;
			throw std::runtime_error("failed to open file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}

	void createCommandPool() {
  	vk::CommandPoolCreateInfo poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
																	 .queueFamilyIndex = queueIndex};

  	commandPool = vk::raii::CommandPool(device, poolInfo);
  }

	void createCommandBuffers() {
		vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
		commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
	}

	void recordCommandBuffer(uint32_t imageIndex) {
		auto &commandBuffer = commandBuffers[frameIndex];
		commandBuffer.begin({});

		transition_image_layout(
		    imageIndex,
		    vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    {},                                                        // srcAccessMask (no need to wait for previous operations)
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
		);
		vk::ClearValue              clearColor     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attachmentInfo = {
		    .imageView   = swapChainImageViews[imageIndex],
		    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eClear,
		    .storeOp     = vk::AttachmentStoreOp::eStore,
		    .clearValue  = clearColor};
		vk::RenderingInfo renderingInfo = {
		    .renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &attachmentInfo};
		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
  	commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
  	commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
		commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
  	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);
  	commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		commandBuffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		transition_image_layout(
		    imageIndex,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    vk::ImageLayout::ePresentSrcKHR,
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
		    {},                                                        // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
		);
		commandBuffer.end();
	}

	void transition_image_layout(
	    uint32_t                imageIndex,
	    vk::ImageLayout         old_layout,
	    vk::ImageLayout         new_layout,
	    vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask)
	{
		vk::ImageMemoryBarrier2 barrier = {
		    .srcStageMask        = src_stage_mask,
		    .srcAccessMask       = src_access_mask,
		    .dstStageMask        = dst_stage_mask,
		    .dstAccessMask       = dst_access_mask,
		    .oldLayout           = old_layout,
		    .newLayout           = new_layout,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image               = swapChainImages[imageIndex],
		    .subresourceRange    = {
		           .aspectMask     = vk::ImageAspectFlagBits::eColor,
		           .baseMipLevel   = 0,
		           .levelCount     = 1,
		           .baseArrayLayer = 0,
		           .layerCount     = 1}};
		vk::DependencyInfo dependency_info = {
		    .dependencyFlags         = {},
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers    = &barrier};
		commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
	}

	void createSyncObjects() {
		assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
			inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
		}
	}

	void drawFrame() {
		auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}
		device.resetFences(*inFlightFences[frameIndex]);


		auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

		commandBuffers[frameIndex].reset();
		recordCommandBuffer(imageIndex);

  	updateUniformBuffer(frameIndex);


		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{.waitSemaphoreCount   = 1,
		                                  .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
		                                  .pWaitDstStageMask    = &waitDestinationStageMask,
		                                  .commandBufferCount   = 1,
		                                  .pCommandBuffers      = &*commandBuffers[frameIndex],
		                                  .signalSemaphoreCount = 1,
		                                  .pSignalSemaphores    = &*renderFinishedSemaphores[imageIndex]};
		graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

		const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
		                                        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
		                                        .swapchainCount     = 1,
		                                        .pSwapchains        = &*swapChain,
		                                        .pImageIndices      = &imageIndex};
		result = graphicsQueue.presentKHR(presentInfoKHR);
  	if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
  	{
			framebufferResized = true;
  		recreateSwapChain();
  	}
  	else
  	{
  		// There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
  		assert(result == vk::Result::eSuccess);
  	}
		frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanupSwapChain() {
  	swapChainImageViews.clear();
  	swapChain = nullptr;
	}

	void recreateSwapChain() {
  	int width = 0, height = 0;
  	glfwGetFramebufferSize(window, &width, &height);

  	while (width == 0 || height == 0) {
  		glfwGetFramebufferSize(window, &width, &height);
  		glfwWaitEvents();
  	}

  	device.waitIdle();

		cleanupSwapChain();

  	createSwapChain();
  	createImageViews();
  }

	void createVertexBuffer()
  {
  	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  	auto [stagingBuffer, stagingBufferMemory] =
				createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  	void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
  	memcpy(dataStaging, vertices.data(), bufferSize);
  	stagingBufferMemory.unmapMemory();

  	std::tie(vertexBuffer, vertexBufferMemory) =
				createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

  	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  }

	void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
  {
  	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
  	commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
  	endSingleTimeCommands(std::move(commandCopyBuffer));
  }

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
  {
  	vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
  	vk::raii::Buffer       buffer          = vk::raii::Buffer(device, bufferInfo);
  	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
  	vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size, .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
  	vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
  	buffer.bindMemory(*bufferMemory, 0);
  	return {std::move(buffer), std::move(bufferMemory)};
  }

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

  	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  	{
  		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
  		{
  			return i;
  		}
  	}

  }

	void createIndexBuffer()
  {
  	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  	auto [stagingBuffer, stagingBufferMemory] =
				createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  	void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  	memcpy(data, indices.data(), (size_t) bufferSize);
  	stagingBufferMemory.unmapMemory();

  	std::tie(indexBuffer, indexBufferMemory) =
				createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

  	copyBuffer(stagingBuffer, indexBuffer, bufferSize);
  }

	void createDescriptorSetLayout() {
  	std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
	    {{.binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex},
			 {.binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment}}};

  	vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};
  	descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

  }

	void createUniformBuffers()
  {
  	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  	{
  		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
  		auto [buffer, bufferMem]  = createBuffer(
					bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
  		uniformBuffers.emplace_back(std::move(buffer));
  		uniformBuffersMemory.emplace_back(std::move(bufferMem));
  		uniformBuffersMapped.emplace_back( uniformBuffersMemory.back().mapMemory(0, bufferSize));
  	}
  }

	void updateUniformBuffer(uint32_t currentImage) {
  	static auto startTime = std::chrono::high_resolution_clock::now();

  	auto currentTime = std::chrono::high_resolution_clock::now();
  	float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  	UniformBufferObject ubo{};
  	ubo.model = glm::rotate(glm::mat4(1.0f), time*glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  	ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  	ubo.proj =
			glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
  	ubo.proj[1][1] *= -1;

  	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

	void createDescriptorPool() {
  	std::array<vk::DescriptorPoolSize, 2> poolSize{{{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT},
																								{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT}}};
  	vk::DescriptorPoolCreateInfo          poolInfo{.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
																									 .maxSets       = MAX_FRAMES_IN_FLIGHT,
																									 .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
																									 .pPoolSizes    = poolSize.data()};
  	descriptorPool = vk::raii::DescriptorPool(device, poolInfo);

  }

	void createDescriptorSets() {
  	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
  	vk::DescriptorSetAllocateInfo        allocInfo{.descriptorPool     = descriptorPool,
																									 .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
																									 .pSetLayouts        = layouts.data()};

  	descriptorSets = device.allocateDescriptorSets(allocInfo);

  	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
  		vk::DescriptorBufferInfo bufferInfo{ .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject) };
  		vk::DescriptorImageInfo  imageInfo{.sampler = textureSampler, .imageView = textureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
  		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{{.dstSet          = descriptorSets[i],
																												 .dstBinding      = 0,
																												 .dstArrayElement = 0,
																												 .descriptorCount = 1,
																												 .descriptorType  = vk::DescriptorType::eUniformBuffer,
																												 .pBufferInfo     = &bufferInfo},
																												{.dstSet          = descriptorSets[i],
																												 .dstBinding      = 1,
																												 .dstArrayElement = 0,
																												 .descriptorCount = 1,
																												 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
																												 .pImageInfo      = &imageInfo}}};
  		device.updateDescriptorSets(descriptorWrites, {});
  	}
  }

	void createTextureImage() {
  	int            texWidth, texHeight, texChannels;
  	stbi_uc       *pixels    = stbi_load("/home/filip/Programs/CLion_Projects/KG_Vulkan/textures/CC0.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  	vk::DeviceSize imageSize = texWidth * texHeight * 4;

  	if (!pixels)
  	{
  		throw std::runtime_error("failed to load texture image!");
  	}

  	auto [stagingBuffer, stagingBufferMemory] =
			createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
  		// createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

  	void* data = stagingBufferMemory.mapMemory(0, imageSize);
  	memcpy(data, pixels, imageSize);
  	stagingBufferMemory.unmapMemory();

  	stbi_image_free(pixels);

  	std::tie(textureImage, textureImageMemory) = createImage(texWidth,
																														 texHeight,
																														 vk::Format::eR8G8B8A8Srgb,
																														 vk::ImageTiling::eOptimal,
																														 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
																														 vk::MemoryPropertyFlagBits::eDeviceLocal);

  	vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
  	transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
  	copyBufferToImage(commandBuffer, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  	transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
  	endSingleTimeCommands(std::move(commandBuffer));
  }

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
			uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
  {
  	vk::ImageCreateInfo imageInfo{.imageType   = vk::ImageType::e2D,
																	.format      = format,
																	.extent      = {width, height, 1},
																	.mipLevels   = 1,
																	.arrayLayers = 1,
																	.samples     = vk::SampleCountFlagBits::e1,
																	.tiling      = tiling,
																	.usage       = usage,
																	.sharingMode = vk::SharingMode::eExclusive};

  	vk::raii::Image image = vk::raii::Image(device, imageInfo);

  	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
  	vk::MemoryAllocateInfo allocInfo{.allocationSize  = memRequirements.size,
																		 .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
  	vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
  	image.bindMemory(imageMemory, 0);

  	return {std::move(image), std::move(imageMemory)};
  }

	void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
  	vk::ImageMemoryBarrier barrier{.oldLayout           = oldLayout,
																	 .newLayout           = newLayout,
																	 .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
																	 .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
																	 .image               = image,
																	 .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}};

  	vk::PipelineStageFlags sourceStage;
  	vk::PipelineStageFlags destinationStage;

  	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
  	{
  		barrier.srcAccessMask = {};
  		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

  		sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
  		destinationStage = vk::PipelineStageFlagBits::eTransfer;
  	}
  	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
  	{
  		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

  		sourceStage      = vk::PipelineStageFlagBits::eTransfer;
  		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  	}
  	else
  	{
  		throw std::invalid_argument("unsupported layout transition!");
  	}
  	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
  }

	void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height){
  	vk::BufferImageCopy region{.bufferOffset      = 0,
													 .bufferRowLength   = 0,
													 .bufferImageHeight = 0,
													 .imageSubresource  = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
													 .imageOffset       = {0, 0, 0},
													 .imageExtent       = {width, height, 1}};

  	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
  }

	vk::raii::CommandBuffer beginSingleTimeCommands()
  {
  	vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
  	vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

  	vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  	commandBuffer.begin(beginInfo);

  	return std::move(commandBuffer);
  }

	void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
  {
  	commandBuffer.end();

  	vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
  	graphicsQueue.submit(submitInfo, nullptr);
  	graphicsQueue.waitIdle();
  }

	void createTextureImageView() {
  	textureImageView = createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb);
  }

	vk::raii::ImageView createImageView(vk::Image const &image, vk::Format format)
  {
  	vk::ImageViewCreateInfo viewInfo{
  		.image            = image,
			.viewType         = vk::ImageViewType::e2D,
			.format           = format,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
  	return vk::raii::ImageView(device, viewInfo);
  }

	void createTextureSampler() {
  	vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
  	vk::SamplerCreateInfo        samplerInfo{.magFilter        = vk::Filter::eLinear,
																						 .minFilter        = vk::Filter::eLinear,
																						 .mipmapMode       = vk::SamplerMipmapMode::eLinear,
																						 .addressModeU     = vk::SamplerAddressMode::eRepeat,
																						 .addressModeV     = vk::SamplerAddressMode::eRepeat,
																						 .addressModeW     = vk::SamplerAddressMode::eRepeat,
																						 .anisotropyEnable = vk::True,
																						 .maxAnisotropy    = properties.limits.maxSamplerAnisotropy};

  	textureSampler = vk::raii::Sampler(device, samplerInfo);
  }






};


int main() {
  try {
    HelloTriangleApplication app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
