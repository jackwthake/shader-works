#!/usr/bin/env python3
import struct
import sys


# this is used to generate the expected values for the bitmap test
def read_bmp_32bit_uncompressed_to_rgb565(filepath):
    """
    Reads a 32-bit uncompressed BMP file and returns its pixel data
    in RGB 565 format as a list of uint16_t values.
    
    Args:
        filepath (str): The path to the 32-bit uncompressed BMP file.
        
    Returns:
        list: A list of uint16_t (Python int) representing pixel data in RGB 565 format.
              Returns an empty list if there's an error or the file is not a valid 32-bit uncompressed BMP.
    """
    pixel_data_rgb565 = []

    try:
        with open(filepath, 'rb') as f:
            # --- Read BITMAPFILEHEADER (14 bytes) ---
            # < : little-endian
            # H : unsigned short (2 bytes) -> bfType
            # I : unsigned int (4 bytes)   -> bfSize
            # H : unsigned short (2 bytes) -> bfReserved1
            # H : unsigned short (2 bytes) -> bfReserved2  <-- CORRECTED from I to H
            # I : unsigned int (4 bytes)   -> bfOffBits
            bfType, bfSize, bfReserved1, bfReserved2, bfOffBits = struct.unpack('<HIHHI', f.read(14))

            if bfType != 0x4D42:  # 'BM'
                print(f"Error: Not a valid BMP file. bfType is 0x{bfType:X}, expected 0x4D42.", file=sys.stderr)
                return []

            # --- Read BITMAPINFOHEADER (40 bytes) ---
            biSize, biWidth, biHeight, biPlanes, biBitCount, biCompression, \
            biSizeImage, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, biClrImportant = \
                struct.unpack('<IIIHHIIIIII', f.read(40))

            print(f"--- BMP File Header ---")
            print(f"bfType: 0x{bfType:X}")
            print(f"bfSize: {bfSize} bytes")
            print(f"bfOffBits: {bfOffBits} bytes")
            print(f"\n--- BMP Info Header ---")
            print(f"biSize: {biSize} bytes")
            print(f"biWidth: {biWidth} pixels")
            print(f"biHeight: {biHeight} pixels (positive for bottom-up, negative for top-down)")
            print(f"biPlanes: {biPlanes}")
            print(f"biBitCount: {biBitCount}")
            print(f"biCompression: {biCompression}")
            print(f"biSizeImage: {biSizeImage} bytes")
            print(f"biXPelsPerMeter: {biXPelsPerMeter}")
            print(f"biYPelsPerMeter: {biYPelsPerMeter}")
            print(f"biClrUsed: {biClrUsed}")
            print(f"biClrImportant: {biClrImportant}")

            # Validate header for 32-bit uncompressed BMP
            if biSize != 40:
                print(f"Warning: biSize is {biSize}, expected 40 for standard BITMAPINFOHEADER.", file=sys.stderr)
            if biBitCount != 32:
                print(f"Error: Not a 32-bit BMP. biBitCount is {biBitCount}.", file=sys.stderr)
                return []
            if biCompression != 0 and biCompression != 3: # BI_RGB (0) or BI_BITFIELDS (3) for 32-bit
                print(f"Error: Only uncompressed (BI_RGB=0) or BITFIELDS (BI_BITFIELDS=3) 32-bit BMPs are supported. biCompression is {biCompression}.", file=sys.stderr)
                return []
            if biCompression == 3:
                print("Warning: BI_BITFIELDS compression detected. This script assumes standard BGRA order for 32-bit pixels and does not read explicit color masks.", file=sys.stderr)
                # To fully support BITFIELDS, you would need to read the bitmasks (Red, Green, Blue, Alpha)
                # that follow the info header if biSize is larger (e.g., BITMAPV4HEADER or BITMAPV5HEADER).
                # For simplicity, we proceed assuming standard BGRA byte order and directly convert.

            # Determine actual height and orientation
            abs_biHeight = abs(biHeight)
            is_bottom_up = (biHeight > 0) # Positive biHeight means bottom-up image

            # Calculate row stride and padding
            bytes_per_pixel = biBitCount // 8
            row_stride_unpadded = biWidth * bytes_per_pixel # Bytes per row without padding
            # BMP rows are padded to be a multiple of 4 bytes
            padding_bytes = (4 - (row_stride_unpadded % 4)) % 4

            print(f"\nCalculations:")
            print(f"Bytes per pixel: {bytes_per_pixel}")
            print(f"Row stride (unpadded): {row_stride_unpadded}")
            print(f"Padding bytes per row: {padding_bytes}")
            print(f"Total rows to process: {abs_biHeight}")

            # Seek to the start of pixel data
            f.seek(bfOffBits)

            # Prepare a list to hold all pixels in top-down, left-to-right order
            # This will store rows temporarily before appending them in the correct order
            rows_data = [[] for _ in range(abs_biHeight)]

            for y in range(abs_biHeight):
                current_row_pixels = []
                for x in range(biWidth):
                    # Read BGRA bytes
                    b = int.from_bytes(f.read(1), byteorder='little')
                    g = int.from_bytes(f.read(1), byteorder='little')
                    r = int.from_bytes(f.read(1), byteorder='little')
                    a = int.from_bytes(f.read(1), byteorder='little') # Alpha byte, read but discarded for RGB565

                    # Convert BGRA (8888) to RGB 565 (16-bit)
                    # RGB 565 format: RRRRRGGG GGGBBBBB
                    # Red: Take top 5 bits of 8-bit R, shift to bits 11-15
                    # Green: Take top 6 bits of 8-bit G, shift to bits 5-10
                    # Blue: Take top 5 bits of 8-bit B, shift to bits 0-4
                    
                    # Ensure values are within 0-255 range and then apply masks
                    r5 = (r >> 3) & 0x1F  # 5 bits for Red
                    g6 = (g >> 2) & 0x3F  # 6 bits for Green
                    b5 = (b >> 3) & 0x1F  # 5 bits for Blue

                    rgb565_pixel = (r5 << 11) | (g6 << 5) | b5
                    current_row_pixels.append(rgb565_pixel)

                # Skip padding bytes at the end of the row
                f.seek(padding_bytes, 1) # 1 means relative to current position

                # Store the current row in the correct order based on biHeight
                if is_bottom_up:
                    # If bottom-up, rows are read from file starting at bottom of image.
                    # Store them in reverse order in our 2D representation.
                    rows_data[abs_biHeight - 1 - y] = current_row_pixels
                else:
                    # If top-down, rows are read starting at top of image.
                    # Store them directly.
                    rows_data[y] = current_row_pixels
            
            # Flatten the 2D list of rows into a single 1D list (top-left to bottom-right)
            for row in rows_data:
                pixel_data_rgb565.extend(row)

    except FileNotFoundError:
        print(f"Error: File not found at '{filepath}'", file=sys.stderr)
        return []
    except struct.error as e:
        print(f"Error reading BMP structure: {e}. File might be corrupted or malformed.", file=sys.stderr)
        return []
    except Exception as e:
        print(f"An unexpected error occurred: {e}", file=sys.stderr)
        return []

    return pixel_data_rgb565

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python bmp_reader.py <path_to_32bit_uncompressed_bmp>", file=sys.stderr)
        sys.exit(1)

    bmp_file_path = sys.argv[1]
    rgb565_pixels = read_bmp_32bit_uncompressed_to_rgb565(bmp_file_path)

    if rgb565_pixels:
        print("\nRGB 565 Pixel Data (comma-separated):")
        # Print the first few pixels for verification, or the whole list if small
        # For very large images, consider writing to a file instead of printing all.
        print(",".join(f"0x{p:04X}" for p in rgb565_pixels))
        # Or if you just want numerical values:
        # print(",".join(str(p) for p in rgb565_pixels))
    else:
        print("\nFailed to process BMP file. Check error messages above.", file=sys.stderr)

