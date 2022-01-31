class FGameTexture;

// Custom, runtime-generated cursors aren't usable on UWP.
bool I_SetCursor(FGameTexture*)
{
	return false;
}