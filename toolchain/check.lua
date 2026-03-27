function main(toolchain)
    if not is_plat("cross") then
        return false
    end

    local devkitpro = os.getenv("DEVKITPRO") or "/opt/devkitpro"
    local devkita64 = path.join(devkitpro, "devkitA64")
    if not os.isdir(devkita64) then
        return false
    end

    toolchain:config_set("devkita64", devkita64)
    toolchain:config_set("bindir", path.join(devkita64, "bin"))
    return true
end