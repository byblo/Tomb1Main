﻿using System.Collections.Generic;

namespace Tomb1Main_ConfigTool.Models;

public class Category
{
    public string ID { get; set; }
    public string Image { get; set; }
    public List<BaseProperty> Properties { get; set; }
    public string Title
    {
        get => Language.Instance.Categories[ID];
    }
}