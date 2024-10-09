#version 330 core

uniform vec3 gridColor;      // Color of the grid lines
uniform vec2 cameraPos;      // Camera position in the xz-plane

out vec4 FragColor;

void main()
{
    // Get the world coordinates of the fragment based on the screen position
    vec2 worldPos = (gl_FragCoord.xy - cameraPos) * 0.1;  // Scale to adjust grid density
    
    // Determine how far the fragment is from the nearest grid line
    float lineThickness = 0.05; // Thickness of the grid lines
    float gridX = abs(fract(worldPos.x) - 0.5);
    float gridY = abs(fract(worldPos.y) - 0.5);

    // If the fragment is within the threshold of a grid line, make it colored; otherwise, make it transparent
    float line = step(min(gridX, gridY), lineThickness);
    vec3 color = mix(vec3(1.0), gridColor, line);
    
    FragColor = vec4(color, 1.0);
}
