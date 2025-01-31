using Installer.Models;
using System;
using System.Windows;
using System.Windows.Controls;

namespace Installer.Controls;

public partial class InstallStepControl : UserControl
{
    public InstallStepControl()
    {
        InitializeComponent();
        DataContextChanged += (object sender, DependencyPropertyChangedEventArgs e) =>
        {
            var dataContext = DataContext as InstallStep;
            if (dataContext is not null)
            {
                string? lastMessage = null;
                dataContext.Logger.LogEvent += (object sender, LogEventArgs e) =>
                {
                    if (e.Message != lastMessage)
                    {
                        lastMessage = e.Message;
                        logTextBox.AppendText(e.Message + Environment.NewLine);
                        logTextBox.Focus();
                        logTextBox.CaretIndex = logTextBox.Text.Length;
                        logTextBox.ScrollToEnd();
                    }
                };
            }
        };
    }
}
