/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/core/Exception.h>
#include <vsg/io/Options.h>
#include <vsg/state/ShaderModule.h>
#include <vsg/traversals/CompileTraversal.h>
#include <SPIRV-Reflect/spirv_reflect.h>

using namespace vsg;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Shader
//
ShaderModule::ShaderModule()
{
}

ShaderModule::ShaderModule(const std::string& in_source) :
    source(in_source)
{
}

ShaderModule::ShaderModule(const SPIRV& in_code) :
    code(in_code)
{
}

ShaderModule::ShaderModule(const std::string& in_source, const SPIRV& in_code) :
    source(in_source),
    code(in_code)
{
}

ShaderModule::~ShaderModule()
{
}

ref_ptr<ShaderModule> ShaderModule::read(const std::string& filename)
{
    SPIRV buffer;
    if (readFile(buffer, filename))
    {
        return ShaderModule::create(buffer);
    }
    else
    {
        throw Exception{"Error: vsg::ShaderModule::read(..) failed to read shader file.", VK_INCOMPLETE};
    }
}

void ShaderModule::read(Input& input)
{
    Object::read(input);

    // TODO review IO
    input.read("Source", source);

    code.resize(input.readValue<uint32_t>("SPIRVSize"));

    input.matchPropertyName("SPIRV");
    input.read(code.size(), code.data());
}

void ShaderModule::write(Output& output) const
{
    Object::write(output);

    output.write("Source", source);

    output.writeValue<uint32_t>("SPIRVSize", code.size());

    output.writePropertyName("SPIRV");
    output.write(code.size(), code.data());
    output.writeEndOfLine();
}

void ShaderModule::compile(Context& context)
{
    if (!_implementation[context.deviceID]) _implementation[context.deviceID] = Implementation::create(context.device, this);
}

const std::map<uint32_t, vsg::DescriptorSetLayoutBindings>& ShaderModule::getDescriptorSetLayoutBindingsMap()
{
    if (!_isReflectDataLoaded) {
        _createReflectData();
    }
    return _descriptorSetLayoutBindingsMap;
}

const vsg::PushConstantRanges& ShaderModule::getPushConstantRanges()
{
    if (!_isReflectDataLoaded) {
        _createReflectData();
    }
    return _pushConstantRanges;
}

void ShaderModule::_createReflectData()
{
    // Create the reflect shader module.
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(code.size() * 4, code.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectCreateShaderModule failed!");
    }


    // Get reflection information on the push constant blocks.
    uint32_t numPushConstantBlocks = 0;
    result = spvReflectEnumeratePushConstantBlocks(&module, &numPushConstantBlocks, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumeratePushConstantBlocks failed!");
    }
    std::vector<SpvReflectBlockVariable*> pushConstantBlockVariables(numPushConstantBlocks);
    result = spvReflectEnumeratePushConstantBlocks(&module, &numPushConstantBlocks, pushConstantBlockVariables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("spvReflectEnumeratePushConstantBlocks failed!");
    }

    _pushConstantRanges.resize(numPushConstantBlocks);
    for (uint32_t blockIdx = 0; blockIdx < numPushConstantBlocks; blockIdx++) {
        VkPushConstantRange& pushConstantRange = _pushConstantRanges.at(blockIdx);
        SpvReflectBlockVariable* pushConstantBlockVariable = pushConstantBlockVariables.at(blockIdx);
        // TODO: Stage flag is set in vsg::ShaderStage.
        pushConstantRange.stageFlags = 0; //uint32_t(shaderModuleType);
        pushConstantRange.offset = pushConstantBlockVariable->absolute_offset;
        pushConstantRange.size = pushConstantBlockVariable->size;
    }


    spvReflectDestroyShaderModule(&module);
}

ShaderModule::Implementation::Implementation(Device* device, ShaderModule* shaderModule) :
    _device(device)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderModule->code.size() * sizeof(ShaderModule::SPIRV::value_type);
    createInfo.pCode = shaderModule->code.data();
    createInfo.pNext = nullptr;

    if (VkResult result = vkCreateShaderModule(*device, &createInfo, _device->getAllocationCallbacks(), &_shaderModule); result != VK_SUCCESS)
    {
        throw Exception{"Error: vsg::ShaderModule::create(...) failed to create shader module.", result};
    }
}

ShaderModule::Implementation::~Implementation()
{
    vkDestroyShaderModule(*_device, _shaderModule, _device->getAllocationCallbacks());
}
