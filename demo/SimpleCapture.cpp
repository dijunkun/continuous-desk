//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED “AS IS? WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#include "pch.h"
#include "SimpleCapture.h"

using namespace winrt;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;

SimpleCapture::SimpleCapture(IDirect3DDevice const &device,
                             GraphicsCaptureItem const &item) {
  m_item = item;
  m_device = device;

  // Set up
  auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
  d3dDevice->GetImmediateContext(m_d3dContext.put());

  auto size = m_item.Size();

  m_swapChain = CreateDXGISwapChain(
      d3dDevice, static_cast<uint32_t>(size.Width),
      static_cast<uint32_t>(size.Height),
      static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 2);

  // Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and
  // frame size.
  m_framePool = Direct3D11CaptureFramePool::Create(
      m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
  m_session = m_framePool.CreateCaptureSession(m_item);
  m_lastSize = size;
  m_frameArrived = m_framePool.FrameArrived(
      auto_revoke, {this, &SimpleCapture::OnFrameArrived});
}

// Start sending capture frames
void SimpleCapture::StartCapture() {
  CheckClosed();
  m_session.StartCapture();
}

ICompositionSurface SimpleCapture::CreateSurface(Compositor const &compositor) {
  CheckClosed();
  return CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

// Process captured frames
void SimpleCapture::Close() {
  auto expected = false;
  if (m_closed.compare_exchange_strong(expected, true)) {
    m_frameArrived.revoke();
    m_framePool.Close();
    m_session.Close();

    m_swapChain = nullptr;
    m_framePool = nullptr;
    m_session = nullptr;
    m_item = nullptr;
  }
}

void SimpleCapture::OnFrameArrived(
    Direct3D11CaptureFramePool const &sender,
    winrt::Windows::Foundation::IInspectable const &) {
  auto newSize = false;

  {
    auto frame = sender.TryGetNextFrame();
    auto frameContentSize = frame.ContentSize();

    if (frameContentSize.Width != m_lastSize.Width ||
        frameContentSize.Height != m_lastSize.Height) {
      // The thing we have been capturing has changed size.
      // We need to resize our swap chain first, then blit the pixels.
      // After we do that, retire the frame and then recreate our frame pool.
      newSize = true;
      m_lastSize = frameContentSize;
      m_swapChain->ResizeBuffers(
          2, static_cast<uint32_t>(m_lastSize.Width),
          static_cast<uint32_t>(m_lastSize.Height),
          static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized),
          0);
    }

    // copy to swapChain
    {
      auto frameSurface =
          GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());


      com_ptr<ID3D11Texture2D> backBuffer;
      check_hresult(m_swapChain->GetBuffer(0, guid_of<ID3D11Texture2D>(),
                                           backBuffer.put_void()));

      m_d3dContext->CopyResource(backBuffer.get(), frameSurface.get());

      DXGI_PRESENT_PARAMETERS presentParameters = {0};
      m_swapChain->Present1(1, 0, &presentParameters);
    }

    // copy to mapped texture
    {
      auto frameSurface =
          GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

      if (!m_mappedTexture || newSize)
        CreateMappedTexture(frameSurface);

      m_d3dContext->CopyResource(m_mappedTexture.get(), frameSurface.get());

      D3D11_MAPPED_SUBRESOURCE mapInfo;
      m_d3dContext->Map(m_mappedTexture.get(), 0, D3D11_MAP_READ,
                        D3D11_MAP_FLAG_DO_NOT_WAIT, &mapInfo);

      // copy data from mapInfo.pData
#if 1
      if (mapInfo.pData) {
        static unsigned char *buffer = nullptr;
        if (buffer && newSize)
          delete[] buffer;

        if (!buffer)
          buffer = new unsigned char[frameContentSize.Width *
                                     frameContentSize.Height * 4];

        int dstRowPitch = frameContentSize.Width * 4;
        for (int h = 0; h < frameContentSize.Height; h++) {
          memcpy_s(buffer + h * dstRowPitch, dstRowPitch,
                   (BYTE *)mapInfo.pData + h * mapInfo.RowPitch,
                   min(mapInfo.RowPitch, dstRowPitch));
        }

        BITMAPINFOHEADER bi;

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = frameContentSize.Width;
        bi.biHeight = frameContentSize.Height * (-1);
        bi.biPlanes = 1;
        bi.biBitCount = 32; // should get from system color bits
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        BITMAPFILEHEADER bf;
        bf.bfType = 0x4d42;
        bf.bfReserved1 = 0;
        bf.bfReserved2 = 0;
        bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bf.bfSize =
            bf.bfOffBits + frameContentSize.Width * frameContentSize.Height * 4;

        FILE *fp = nullptr;

        fopen_s(&fp, ".\\save.bmp", "wb+");

        fwrite(&bf, 1, sizeof(bf), fp);
        fwrite(&bi, 1, sizeof(bi), fp);
        fwrite(buffer, 1, frameContentSize.Width * frameContentSize.Height * 4,
               fp);

        fflush(fp);
        fclose(fp);
      }
#endif

      m_d3dContext->Unmap(m_mappedTexture.get(), 0);
    }
  }

  if (newSize) {
    m_framePool.Recreate(m_device, DirectXPixelFormat::B8G8R8A8UIntNormalized,
                         2, m_lastSize);
  }
}

HRESULT
SimpleCapture::CreateMappedTexture(winrt::com_ptr<ID3D11Texture2D> src_texture,
                                   UINT width, UINT height) {
  D3D11_TEXTURE2D_DESC src_desc;
  src_texture->GetDesc(&src_desc);
  D3D11_TEXTURE2D_DESC map_desc;
  map_desc.Width = width == 0 ? src_desc.Width : width;
  map_desc.Height = height == 0 ? src_desc.Height : height;
  map_desc.MipLevels = src_desc.MipLevels;
  map_desc.ArraySize = src_desc.ArraySize;
  map_desc.Format = src_desc.Format;
  map_desc.SampleDesc = src_desc.SampleDesc;
  map_desc.Usage = D3D11_USAGE_STAGING;
  map_desc.BindFlags = 0;
  map_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  map_desc.MiscFlags = 0;

  auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);

  return d3dDevice->CreateTexture2D(&map_desc, nullptr, m_mappedTexture.put());
}
