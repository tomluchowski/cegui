/***********************************************************************
    filename:   Shader.cpp
    created:    Sun, 6th April 2014
    author:     Lukas E Meindl
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2014 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/

#include "CEGUI/RendererModules/Direct3D11/Shader.h"
#include "CEGUI/Logger.h"
#include "CEGUI/Exceptions.h"

#include <D3Dcompiler.h>


#include <sstream>
#include <iostream>

namespace CEGUI
{
//----------------------------------------------------------------------------//
static const size_t LOG_BUFFER_SIZE = 8096;

//----------------------------------------------------------------------------//
Direct3D11Shader::Direct3D11Shader(Direct3D11Renderer& owner,
                                   const std::string& vertexShaderSource,
                                   const std::string& pixelShaderSource)
    : d_device(owner.getDirect3DDevice())
    , d_deviceContext(owner.getDirect3DDeviceContext())
    , d_vertShader(0)
    , d_pixelShader(0)
    , d_vertexShaderBuffer(0)
    , d_pixelShaderBuffer(0)
    , d_vertexShaderReflection(0)
    , d_pixelShaderReflection(0)
{
    createVertexShader(vertexShaderSource);
    createPixelShader(pixelShaderSource);
}

//----------------------------------------------------------------------------//
Direct3D11Shader::~Direct3D11Shader()
{
    if(d_vertexShaderBuffer)
        d_vertexShaderBuffer->Release();

    if(d_vertexShaderReflection)
        d_vertexShaderReflection->Release();

    if(d_pixelShaderBuffer)
        d_pixelShaderBuffer->Release(); 

    if(d_pixelShaderReflection)
        d_pixelShaderReflection->Release();
}

//----------------------------------------------------------------------------//
void Direct3D11Shader::bind() const
{
    //Set Vertex and Pixel Shaders
	d_deviceContext->VSSetShader(d_vertShader, 0, 0);
	d_deviceContext->PSSetShader(d_pixelShader, 0, 0);
    d_deviceContext->CSSetShader(0, 0, 0);
    d_deviceContext->DSSetShader(0, 0, 0);
    d_deviceContext->GSSetShader(0, 0, 0);
}

//----------------------------------------------------------------------------//
void Direct3D11Shader::createVertexShader(const std::string& vertexShaderSource)
{
    HRESULT result;
    ID3D10Blob* errorMessage;

    UINT flags1 = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

#if defined(DEBUG) || defined (_DEBUG)
        flags1 |= D3DCOMPILE_DEBUG;
#else
        flags1 |= 0;
#endif

    result = D3DCompile(static_cast<LPCSTR>(vertexShaderSource.c_str()), vertexShaderSource.size(), "D3D11 Vertex Shader",
                        0, 0, "VSMain", "vs_4_0", flags1, 0, &d_vertexShaderBuffer, &errorMessage);
    if(FAILED(result))
    {
		std::string msg(static_cast<const char*>(errorMessage->GetBufferPointer()), errorMessage->GetBufferSize());

		errorMessage->Release();
		CEGUI_THROW(RendererException(msg));
    }


    result = d_device->CreateVertexShader(d_vertexShaderBuffer->GetBufferPointer(), d_vertexShaderBuffer->GetBufferSize(), 0, &d_vertShader);
    if(FAILED(result))
    {
        std::string msg("Direct3D11Shader: Failed to create the vertex shader");
		CEGUI_THROW(RendererException(msg));
    }

    result = D3DReflect(d_vertexShaderBuffer->GetBufferPointer(), d_vertexShaderBuffer->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&d_vertexShaderReflection));
    if(FAILED(result))
    {
        std::string msg("D3DReflect: Shader reflection failed");
		CEGUI_THROW(RendererException(msg));
    }
}

//----------------------------------------------------------------------------//
void Direct3D11Shader::createPixelShader(const std::string& pixelShaderSource)
{
    HRESULT result;
    ID3D10Blob* errorMessage;

    UINT flags1 = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

#if defined(DEBUG) || defined (_DEBUG)
        flags1 |= D3DCOMPILE_DEBUG;
#else
        flags1 |= 0;
#endif

    result = D3DCompile(static_cast<LPCSTR>(pixelShaderSource.c_str()), pixelShaderSource.size(), "D3D11 Pixel Shader",
                        0, 0, "PSMain", "ps_4_0", flags1, 0, &d_pixelShaderBuffer, &errorMessage);
    if(FAILED(result))
    {
		std::string msg(static_cast<const char*>(errorMessage->GetBufferPointer()), errorMessage->GetBufferSize());

		errorMessage->Release();
		CEGUI_THROW(RendererException(msg));
    }


    result = d_device->CreatePixelShader(d_pixelShaderBuffer->GetBufferPointer(), d_pixelShaderBuffer->GetBufferSize(), 0, &d_pixelShader);
    if(FAILED(result))
    {
        std::string msg("Direct3D11Shader: Failed to create the pixel shader");
		CEGUI_THROW(RendererException(msg));
    }

    result = D3DReflect(d_pixelShaderBuffer->GetBufferPointer(), d_pixelShaderBuffer->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&d_pixelShaderReflection));
    if(FAILED(result))
    {
        std::string msg("D3DReflect: Shader reflection failed");
		CEGUI_THROW(RendererException(msg));
    }
}

//----------------------------------------------------------------------------//
D3D11_SHADER_INPUT_BIND_DESC Direct3D11Shader::getTextureBindingDesc(const std::string& variableName, ShaderType shaderType)
{
    HRESULT result;

    ID3D11ShaderReflection* shaderReflection = 0;
    if(shaderType == ST_VERTEX)
        shaderReflection = d_vertexShaderReflection;
    else if(shaderType == ST_PIXEL)
        shaderReflection = d_pixelShaderReflection;

    D3D11_SHADER_DESC shaderDescription;
    result = shaderReflection->GetDesc(&shaderDescription);
    if(FAILED(result))
    {
        std::string errorMessage("ID3D11ShaderReflection::GetDesc() call failed. Could not retrieve shader description.");
		CEGUI_THROW(RendererException(errorMessage));
    }

    UINT resourceCount = shaderDescription.BoundResources;
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;

    for(UINT i = 0; i < resourceCount; ++i)
    {
        result = shaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc);
        if(SUCCEEDED(result))
        {
            int comparisonResult = variableName.compare(shaderInputBindDesc.Name);
            if(comparisonResult == 0)
                return shaderInputBindDesc;
        }
    }

    std::string errorMessage = std::string("Variable was not found in shader for variable of name \"") + variableName + "\"";
    CEGUI_THROW(RendererException(errorMessage));

    return shaderInputBindDesc;
}


//----------------------------------------------------------------------------//
D3D11_SHADER_VARIABLE_DESC Direct3D11Shader::getUniformVariableDescription(const std::string& variableName, ShaderType shaderType)
{
    HRESULT result;

    ID3D11ShaderReflectionConstantBuffer* shaderReflectConstBuffer = getShaderReflectionConstBuffer(shaderType);

    ID3D11ShaderReflectionVariable* shaderReflectVariable = shaderReflectConstBuffer->GetVariableByName(variableName.c_str());

    D3D11_SHADER_VARIABLE_DESC varDesc;
    result = shaderReflectVariable->GetDesc(&varDesc);
    if(FAILED(result))
        CEGUI_THROW(RendererException("Could not retrieve variable \"" + variableName + "\" from the buffer \"$Globals\""));

    return varDesc;
}

//----------------------------------------------------------------------------//
ID3D11ShaderReflectionConstantBuffer* Direct3D11Shader::getShaderReflectionConstBuffer(ShaderType shaderType)
{
    HRESULT result;

    ID3D11ShaderReflection* shaderReflection = 0;
    if(shaderType == ST_VERTEX)
        shaderReflection = d_vertexShaderReflection;
    else if(shaderType == ST_PIXEL)
        shaderReflection = d_pixelShaderReflection;

    D3D11_SHADER_DESC shaderDescription;
    result = shaderReflection->GetDesc(&shaderDescription);
    if(FAILED(result))
    {
        std::string errorMessage("ID3D11ShaderReflection::GetDesc() call failed. Could not retrieve shader description.");
		CEGUI_THROW(RendererException(errorMessage));
    }

    ID3D11ShaderReflectionConstantBuffer* shaderReflectConstBufferGlobals = shaderReflection->GetConstantBufferByName("$Globals");
    if(shaderReflectConstBufferGlobals == 0)
        CEGUI_THROW(RendererException("Could not retrieve constant buffer \"$Globals\" from the shader"));
    
    return shaderReflectConstBufferGlobals;
}

//----------------------------------------------------------------------------//
ID3D10Blob* Direct3D11Shader::getVertexShaderBuffer() const
{
    return d_vertexShaderBuffer;
}

//----------------------------------------------------------------------------//
}