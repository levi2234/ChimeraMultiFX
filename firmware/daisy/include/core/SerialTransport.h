#pragma once

#include "daisy_seed.h"

class SerialTransport {
public:
    static constexpr size_t RX_QUEUE_LEN = 512;
    using ByteCallback = void (*)(char byte, void* context);

    void Init(daisy::UsbHandle* usb, daisy::UartHandler* uart) {
        usb_ = usb;
        uart_ = uart;
    }

    void StartUartDmaReceive(
        uint8_t* buffer,
        size_t size,
        daisy::UartHandler::CircularRxCallbackFunctionPtr callback,
        void* context) {
        uart_rx_dma_buffer_ = buffer;
        uart_rx_dma_buffer_size_ = size;
        uart_rx_dma_callback_ = callback;
        uart_rx_dma_context_ = context;
        if (uart_) uart_->DmaListenStart(buffer, size, callback, context);
    }

    void QueueUartBytes(const uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            const uint16_t next = static_cast<uint16_t>((rx_head_ + 1) % RX_QUEUE_LEN);
            if (next == rx_tail_) {
                rx_overflowed_ = true;
                continue;
            }
            rx_queue_[rx_head_] = data[i];
            rx_head_ = next;
        }
    }

    void ProcessPending(ByteCallback callback, void* context) {
        if (!callback) return;
        char byte = 0;
        while (Dequeue(byte)) callback(byte, context);
    }

    bool TakeRxOverflow() {
        const bool overflowed = rx_overflowed_;
        rx_overflowed_ = false;
        return overflowed;
    }

    void Send(const char* data, size_t size) {
        if (size == 0) return;
        if (usb_) {
            usb_->TransmitInternal(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), size);
        }
        if (!uart_) return;

        // libDaisy keeps UART TX busy while circular RX DMA is active.
        uart_->DmaListenStop();
        uart_->BlockingTransmit(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), size, 2000);
        if (uart_rx_dma_buffer_ && uart_rx_dma_callback_) {
            uart_->DmaListenStart(
                uart_rx_dma_buffer_, uart_rx_dma_buffer_size_,
                uart_rx_dma_callback_, uart_rx_dma_context_);
        }
    }

private:
    bool Dequeue(char& byte) {
        if (rx_tail_ == rx_head_) return false;
        byte = static_cast<char>(rx_queue_[rx_tail_]);
        rx_tail_ = static_cast<uint16_t>((rx_tail_ + 1) % RX_QUEUE_LEN);
        return true;
    }

    daisy::UsbHandle* usb_ = nullptr;
    daisy::UartHandler* uart_ = nullptr;
    uint8_t rx_queue_[RX_QUEUE_LEN] = {};
    volatile uint16_t rx_head_ = 0;
    volatile uint16_t rx_tail_ = 0;
    volatile bool rx_overflowed_ = false;
    uint8_t* uart_rx_dma_buffer_ = nullptr;
    size_t uart_rx_dma_buffer_size_ = 0;
    daisy::UartHandler::CircularRxCallbackFunctionPtr uart_rx_dma_callback_ = nullptr;
    void* uart_rx_dma_context_ = nullptr;
};