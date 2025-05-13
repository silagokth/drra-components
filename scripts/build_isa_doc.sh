#!/bin/bash

# Check if jq is installed
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required but not installed. Please install jq."
    exit 1
fi

# Default output file path
DEFAULT_OUTPUT_FILE="./InstructionSet.md"

# Get output file path from command-line argument or use default
OUTPUT_FILE="${1:-$DEFAULT_OUTPUT_FILE}"

# Define temporary directory
TEMP_DIR=$(mktemp -d)

echo "Cloning repository to $TEMP_DIR..."
git clone git@github.com:silagokth/drra-components.git "$TEMP_DIR" --quiet

# Check if clone was successful
if [ $? -ne 0 ]; then
    echo "Error: Failed to clone repository"
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "Repository cloned successfully. Finding and processing ISA files..."

# Create header for the markdown file
cat > "$OUTPUT_FILE" << 'EOT'
# ISA Specification

Instructions are 32-bit wide. The MSB indicates whether it's a control instruction or a resource instruction. [0]: control; [1]: resource; the next 3 bits represent the instruction opcode.
The rest of the bits are used to encode the instruction content. For resource instructions, another 4 bits in the instruction content are used to indicate the slot number.
The rest of the bits are used to encode the instruction content.

Note that for resource instructions, if the instruction opcode starts with "11", the instruction contains a field that needs to be replaced by scalar registers if the field is marked as "dynamic".

## ISA Format

| Parameter             | Width | Description                                                  |
| --------------------- | ----- | ------------------------------------------------------------ |
| instr_bitwidth        | 32    | Instruction bitwidth                                         |
| instr_type_bitwidth   | 1     | Instruction type bitwidth                                    |
| instr_opcode_bitwidth | 3     | Instruction opcode bitwidth                                  |
| instr_slot_bitwidth   | 4     | Instruction slot bitwidth, only used for resource components |

## Instructions For Each Component

EOT

# Count how many isa.json files we found
ISA_FILE_COUNT=$(find "$TEMP_DIR" -name "isa.json" | wc -l)
echo "Found $ISA_FILE_COUNT isa.json files to process."

if [ "$ISA_FILE_COUNT" -eq 0 ]; then
    echo "Warning: No isa.json files found in the repository."
    echo "Make sure the repository structure hasn't changed."
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Find all isa.json files and process them
find "$TEMP_DIR" -name "isa.json" | sort | while read -r isa_file; do
    # Get the parent directory name as component name
    component_name=$(basename "$(dirname "$isa_file")")
    
    # Get component type from the parent directory of the component directory (2 levels up)
    component_dir=$(dirname "$isa_file")
    component_type_dir=$(dirname "$component_dir")
    component_type=$(basename "$component_type_dir")
    component_type=${component_type%s}
    
    echo "Processing $component_name ($component_type)..."
    
    # Check if the file has instructions and the array is not empty
    if ! jq -e '.instructions and (.instructions | length > 0)' "$isa_file" > /dev/null 2>&1; then
        echo "Warning: No instructions found in $isa_file or instructions array is empty. Skipping..."
        continue
    else
        # Write component header to the markdown file
        echo "### $component_name ($component_type)" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
    fi
    
    # Process instructions in the isa.json file
    jq -c '.instructions[]?' "$isa_file" 2>/dev/null | while read -r instr; do
        if [ -z "$instr" ] || [ "$instr" = "null" ]; then
            continue
        fi
        
        name=$(echo "$instr" | jq -r '.name // "unknown"')
        opcode=$(echo "$instr" | jq -r '.opcode // "?"')
        
        echo "#### $name [opcode=$opcode]" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        
        # Create table header for fields
        echo "| Field | Position | Width | Default Value | Description |" >> "$OUTPUT_FILE"
        echo "| ----- | -------- | ----- | ------------- | ----------- |" >> "$OUTPUT_FILE"
        
        # Check if the instruction has segments or fields
        has_segments=$(echo "$instr" | jq 'has("segments")')
        has_fields=$(echo "$instr" | jq 'has("fields")')
        
        if [ "$has_segments" = "true" ]; then
            # Get instruction format information
            instr_type_width=$(jq -r '.format.instr_type_bitwidth // 1' "$isa_file")
            opcode_width=$(jq -r '.format.instr_opcode_bitwidth // 3' "$isa_file")
            slot_width=$(jq -r '.format.instr_slot_bitwidth // 4' "$isa_file")
            
            # Calculate starting position (after type, opcode, and possibly slot)
            start_pos=$(( 32 - 1 - opcode_width ))
            instr_type=$(echo "$instr" | jq -r '.instr_type // 0')
            
            # If it's a resource instruction (type 1), account for slot bits
            if [ "$instr_type" = "1" ]; then
                start_pos=$(( start_pos - slot_width ))
            fi
            
            # Process each segment - we'll use a temporary file to store segments
            field_count=0
            echo "$instr" | jq -c '.segments[]' > "$TEMP_DIR/segments.json"
            
            # Process each segment line by line
            while IFS= read -r segment_json; do
                ((field_count++))
                
                # Extract segment properties
                name=$(echo "$segment_json" | jq -r '.name // ""')
                width=$(echo "$segment_json" | jq -r '.bitwidth // 0')
                comment=$(echo "$segment_json" | jq -r '.comment // ""')
                default_val=$(echo "$segment_json" | jq -r '.default_val // 0')
                
                # Calculate position based on bitwidth
                high_pos=$start_pos
                low_pos=$(( start_pos - width + 1 ))
                position="[$high_pos, $low_pos]"
                
                # Adjust starting position for next segment
                start_pos=$(( low_pos - 1 ))
                
                # Check for verbose mappings
                if echo "$segment_json" | jq -e 'has("verbo_map")' > /dev/null 2>&1; then
                    # Create a temporary file for the verbose mappings
                    echo "$segment_json" | jq -c '.verbo_map[]' > "$TEMP_DIR/mappings.json"
                    
                    map_desc=""
                    while IFS= read -r mapping_json; do
                        key=$(echo "$mapping_json" | jq -r '.key')
                        val=$(echo "$mapping_json" | jq -r '.val')
                        map_desc="$map_desc [$key]: $val;"
                    done < "$TEMP_DIR/mappings.json"
                    
                    if [ -n "$map_desc" ]; then
                        comment="$comment $map_desc"
                    fi
                fi
                
                echo "| $name | $position | $width | $default_val | $comment |" >> "$OUTPUT_FILE"
            done < "$TEMP_DIR/segments.json"
            
            # If no fields were processed, add an empty row
            if [ "$field_count" -eq 0 ]; then
                echo "| | | | | |" >> "$OUTPUT_FILE"
            fi
            
        elif [ "$has_fields" = "true" ]; then
            # Handle fields format as before
            field_count=0
            echo "$instr" | jq -c '.fields[]' > "$TEMP_DIR/fields.json"
            
            while IFS= read -r field_json; do
                ((field_count++))
                
                field_name=$(echo "$field_json" | jq -r '.name // ""')
                
                # Handle different position formats
                position=""
                if echo "$field_json" | jq -e 'has("position")' > /dev/null 2>&1; then
                    position_format=$(echo "$field_json" | jq -r 'if .position | type == "object" then "object" elif .position | type == "array" then "array" else "other" end')
                    
                    if [ "$position_format" = "object" ]; then
                        position_hi=$(echo "$field_json" | jq -r '.position.hi // 0')
                        position_lo=$(echo "$field_json" | jq -r '.position.lo // 0')
                        position="[$position_hi, $position_lo]"
                    elif [ "$position_format" = "array" ]; then
                        position_hi=$(echo "$field_json" | jq -r '.position[0] // 0')
                        position_lo=$(echo "$field_json" | jq -r '.position[1] // 0')
                        position="[$position_hi, $position_lo]"
                    else
                        position=$(echo "$field_json" | jq -r '.position')
                    fi
                else
                    position="[]"
                fi
                
                width=$(echo "$field_json" | jq -r '.width // ""')
                default=$(echo "$field_json" | jq -r '.default // "0"')
                description=$(echo "$field_json" | jq -r '.description // ""')
                
                echo "| $field_name | $position | $width | $default | $description |" >> "$OUTPUT_FILE"
            done < "$TEMP_DIR/fields.json"
            
            # If no fields were processed, add an empty row
            if [ "$field_count" -eq 0 ]; then
                echo "| | | | | |" >> "$OUTPUT_FILE"
            fi
        else
            # No segments or fields
            echo "| | | | | |" >> "$OUTPUT_FILE"
        fi
        
        echo "" >> "$OUTPUT_FILE"
    done
done

echo "Cleaning up temporary directory..."
rm -rf "$TEMP_DIR"

echo "ISA documentation generated successfully at $OUTPUT_FILE"
chmod +x "$0"
