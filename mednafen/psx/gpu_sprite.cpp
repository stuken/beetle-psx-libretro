
template<bool textured, int BlendMode, bool TexMult, uint32_t TexMode_TA,
   bool MaskEval_TA, bool FlipX, bool FlipY>
static void DrawSprite(PS_GPU *gpu, int32_t x_arg, int32_t y_arg, int32_t w, int32_t h,
      uint8_t u_arg, uint8_t v_arg, uint32_t color, uint32_t clut_offset)
{
   uint8_t u, v;
   const int32_t r           = color & 0xFF;
   const int32_t g           = (color >> 8) & 0xFF;
   const int32_t b           = (color >> 16) & 0xFF;
   const uint16_t fill_color = 0x8000 | ((r >> 3) << 0) | ((g >> 3) << 5) | ((b >> 3) << 10);
   int32_t v_inc             = 1;
   int32_t u_inc             = 1;
   int32_t x_start           = x_arg;
   int32_t x_bound           = x_arg + w;
   int32_t y_start           = y_arg;
   int32_t y_bound           = y_arg + h;

   //printf("[GPU] Sprite: x=%d, y=%d, w=%d, h=%d\n", x_arg, y_arg, w, h);

   if(textured)
   {
      u = u_arg;
      v = v_arg;

      //if(FlipX || FlipY || (u & 1) || (v & 1) || ((TexMode_TA == 0) && ((u & 3) || (v & 3))))
      // fprintf(stderr, "Flippy: %d %d 0x%02x 0x%02x\n", FlipX, FlipY, u, v);

      if(FlipX)
      {
         u_inc = -1;
         u |= 1;
      }
      // FIXME: Something weird happens when lower bit of u is set and we're not doing horizontal flip, but I'm not sure what it is exactly(needs testing)
      // It may only happen to the first pixel, so look for that case too during testing.
      //else
      // u = (u + 1) & ~1;

      if(FlipY)
         v_inc = -1;
   }

   if(x_start < gpu->ClipX0)
   {
      if(textured)
         u += (gpu->ClipX0 - x_start) * u_inc;

      x_start = gpu->ClipX0;
   }

   if(y_start < gpu->ClipY0)
   {
      if(textured)
         v += (gpu->ClipY0 - y_start) * v_inc;

      y_start = gpu->ClipY0;
   }

   if(x_bound > (gpu->ClipX1 + 1))
      x_bound = gpu->ClipX1 + 1;

   if(y_bound > (gpu->ClipY1 + 1))
      y_bound = gpu->ClipY1 + 1;

   //HeightMode && !dfe && ((y & 1) == ((DisplayFB_YStart + !field_atvs) & 1)) && !DisplayOff
   //printf("%d:%d, %d, %d ---- heightmode=%d displayfb_ystart=%d field_atvs=%d displayoff=%d\n", w, h, scanline, dfe, HeightMode, DisplayFB_YStart, field_atvs, DisplayOff);

   for(int32_t y = y_start; MDFN_LIKELY(y < y_bound); y++)
   {
      uint8_t u_r;

      if(textured)
         u_r = u;

      if(!LineSkipTest(gpu, y))
      {
         if(y_bound > y_start && x_bound > x_start)
         {
            /* Note(TODO): From tests on a PS1, 
             * even a 0-width sprite takes up time 
             * to "draw" proportional to its height. */
            int32_t suck_time = /* 8 + */ (x_bound - x_start);

            if((BlendMode >= 0) || MaskEval_TA)
               suck_time += (((x_bound + 1) & ~1) - (x_start & ~1)) >> 1;

            gpu->DrawTimeAvail -= suck_time;
         }

         for(int32_t x = x_start; MDFN_LIKELY(x < x_bound); x++)
         {
            if(textured)
            {
               uint16_t fbw = gpu->GetTexel<TexMode_TA>(clut_offset, u_r, v);

               if(fbw)
               {
                  if(TexMult)
                     fbw = gpu->ModTexel(fbw, r, g, b, 3, 2);
                  gpu->PlotNativePixel<BlendMode, MaskEval_TA, true>(x, y, fbw);
               }
            }
            else
               gpu->PlotNativePixel<BlendMode, MaskEval_TA, false>(x, y, fill_color);

            if(textured)
               u_r += u_inc;
         }
      }
      if(textured)
         v += v_inc;
   }
}

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
void rsx_gl_push_sprite(
      int16_t p0x,
      int16_t p0y,
      int16_t p1x,
      int16_t p1y,
      int16_t p2x,
      int16_t p2y,
      int16_t p3x,
      int16_t p3y,
      uint32_t c0,
      uint32_t c1,
      uint32_t c2,
      uint32_t c3,
      uint16_t t0x,
      uint16_t t0y,
      uint16_t t1x,
      uint16_t t1y,
      uint16_t t2x,
      uint16_t t2y,
      uint16_t t3x,
      uint16_t t3y,
      uint16_t texpage_x,
      uint16_t texpage_y,
      uint16_t clut_x,
      uint16_t clut_y,
      uint8_t texture_blend_mode,
      uint8_t depth_shift,
      bool dither,
      int blend_mode);
#endif

template<uint8_t raw_size, bool textured, int BlendMode,
   bool TexMult, uint32_t TexMode_TA, bool MaskEval_TA>
static void Command_DrawSprite(PS_GPU *gpu, const uint32_t *cb)
{
   int32_t x, y;
   int32_t w, h;
   uint8_t     u    = 0;
   uint8_t     v    = 0;
   uint32_t color   = 0;
   uint32_t clut    = 0;

   gpu->DrawTimeAvail   -= 16;	// FIXME, correct time.

   color            = *cb & 0x00FFFFFF;
   cb++;

   x                = sign_x_to_s32(11, (*cb & 0xFFFF));
   y                = sign_x_to_s32(11, (*cb >> 16));
   cb++;

   if(textured)
   {
      u    = *cb & 0xFF;
      v    = (*cb >> 8) & 0xFF;
      clut = ((*cb >> 16) & 0xFFFF) << 4;
      gpu->Update_CLUT_Cache<TexMode_TA>((*cb >> 16) & 0xFFFF);
      cb++;
   }

   switch(raw_size)
   {
      default:
      case 0:
         w = (*cb & 0x3FF);
         h = (*cb >> 16) & 0x1FF;
         cb++;
         break;

      case 1:
         w = 1;
         h = 1;
         break;

      case 2:
         w = 8;
         h = 8;
         break;

      case 3:
         w = 16;
         h = 16;
         break;
   }

   x = sign_x_to_s32(11, x + gpu->OffsX);
   y = sign_x_to_s32(11, y + gpu->OffsY);

   uint16_t clut_x = (clut & (0x3f << 4));
   uint16_t clut_y = (clut >> 10) & 0x1ff;

   enum blending_modes blend_mode = BLEND_MODE_AVERAGE;

   if (textured)
   {
     if (TexMult)
       blend_mode = BLEND_MODE_SUBTRACT;
     else
       blend_mode = BLEND_MODE_ADD;
   }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) || defined(HAVE_VULKAN)
   if (rsx_intf_is_type() == RSX_OPENGL || rsx_intf_is_type() == RSX_VULKAN)
   {
	   rsx_intf_push_quad(
		   x,                /* p0x */
		   y,                /* p0y */
		   1,
		   x + w,            /* p1x */
		   y,                /* p1y */
		   1,
		   x,                /* p2x */
		   y + h,            /* p2y */
		   1,
		   x + w,            /* p3x */
		   y + h,            /* p3y */
		   1,
		   color,            /* c0 */
		   color,            /* c1 */
		   color,            /* c2 */
		   color,            /* c3 */
		   u,                /* t0x */
		   v,                /* t0y */
		   u + w,            /* t1x */
		   v,                /* t1y */
		   u,                /* t2x */
		   v + h,            /* t2y */
		   u + w,            /* t5x */
		   v + h,            /* t5y */
            gpu->TexPageX,
            gpu->TexPageY,
            clut_x,
            clut_y,
            blend_mode,
            2 - TexMode_TA,
            gpu->DitherEnabled(),
            BlendMode,
            MaskEval_TA,
            gpu->MaskSetOR != 0);
   }
   else
#endif
   {
      rsx_intf_push_triangle(x, y, 1,
            x + w, y, 1,
            x, y + h, 1,
            color,
            color,
            color,
            u, v,
            u + w, v,
            u, v + h,
            gpu->TexPageX,
            gpu->TexPageY,
            clut_x, clut_y,
            blend_mode,
            2 - TexMode_TA,
            gpu->DitherEnabled(),
            BlendMode,
            MaskEval_TA,
            gpu->MaskSetOR != 0);

      rsx_intf_push_triangle(x + w, y, 1,
            x, y + h, 1,
            x + w, y + h, 1,
            color,
            color,
            color,
            u + w, v,
            u, v + h,
            u + w, v + h,
            gpu->TexPageX,
            gpu->TexPageY,
            clut_x, clut_y,
            blend_mode,
            2 - TexMode_TA,
            gpu->DitherEnabled(),
            BlendMode,
            MaskEval_TA,
            gpu->MaskSetOR != 0);
   }

#if 0
   printf("SPRITE: %d %d %d -- %d %d\n", raw_size, x, y, w, h);
#endif

   if (!rsx_intf_has_software_renderer())
      return;

   switch(gpu->SpriteFlip & 0x3000)
   {
      case 0x0000:
         if(!TexMult || color == 0x808080)
            DrawSprite<textured, BlendMode, false, TexMode_TA, MaskEval_TA, false, false>(gpu, x, y, w, h, u, v, color, clut);
         else
            DrawSprite<textured, BlendMode, true, TexMode_TA, MaskEval_TA, false, false>(gpu, x, y, w, h, u, v, color, clut);
         break;

      case 0x1000:
         if(!TexMult || color == 0x808080)
            DrawSprite<textured, BlendMode, false, TexMode_TA, MaskEval_TA, true, false>(gpu, x, y, w, h, u, v, color, clut);
         else
            DrawSprite<textured, BlendMode, true, TexMode_TA, MaskEval_TA, true, false>(gpu, x, y, w, h, u, v, color, clut);
         break;

      case 0x2000:
         if(!TexMult || color == 0x808080)
            DrawSprite<textured, BlendMode, false, TexMode_TA, MaskEval_TA, false, true>(gpu, x, y, w, h, u, v, color, clut);
         else
            DrawSprite<textured, BlendMode, true, TexMode_TA, MaskEval_TA, false, true>(gpu, x, y, w, h, u, v, color, clut);
         break;

      case 0x3000:
         if(!TexMult || color == 0x808080)
            DrawSprite<textured, BlendMode, false, TexMode_TA, MaskEval_TA, true, true>(gpu, x, y, w, h, u, v, color, clut);
         else
            DrawSprite<textured, BlendMode, true, TexMode_TA, MaskEval_TA, true, true>(gpu, x, y, w, h, u, v, color, clut);
         break;
   }
}
